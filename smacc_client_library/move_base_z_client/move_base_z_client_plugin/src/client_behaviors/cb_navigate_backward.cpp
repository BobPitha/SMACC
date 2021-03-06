#include <move_base_z_client_plugin/client_behaviors/cb_navigate_backward.h>
#include <thread>
#include <tf/tf.h>


namespace cl_move_base_z
{
using namespace ::cl_move_base_z::odom_tracker;

CbNavigateBackwards::CbNavigateBackwards(float backwardDistance)
{
    if (backwardDistance < 0)
    {
        ROS_ERROR("cb backward: distance must be greater or equal than 0");
        this->backwardDistance = 0;
    }
    this->backwardDistance = backwardDistance;
}

CbNavigateBackwards::CbNavigateBackwards()
{
}

void CbNavigateBackwards::onEntry()
{
    // straight motion distance
    double dist;

    if (!backwardDistance)
    {
        this->getCurrentState()->param("straight_motion_distance", dist, 3.5);
    }
    else
    {
        dist = *backwardDistance;
    }

    ROS_INFO_STREAM("Straight motion distance: " << dist);

    this->requiresClient(moveBaseClient_);

    //this should work better with a coroutine and await
    ros::Rate rate(10.0);
    tf::StampedTransform currentPose;

    while (ros::ok())
    {
        try
        {
            listener.lookupTransform("/odom", "/base_link",
                                     ros::Time(0), currentPose);

            break;
        }
        catch (tf::TransformException ex)
        {
            ROS_INFO("[CbNavigateBackward] Waiting transform: %s", ex.what());
            ros::Duration(1.0).sleep();
        }
    }

    tf::Transform forwardDeltaTransform;
    forwardDeltaTransform.setIdentity();
    forwardDeltaTransform.setOrigin(tf::Vector3(-dist, 0, 0));

    tf::Transform targetPose = currentPose * forwardDeltaTransform;

    ClMoveBaseZ::Goal goal;
    goal.target_pose.header.frame_id = "/odom";
    goal.target_pose.header.stamp = ros::Time::now();
    tf::poseTFToMsg(targetPose, goal.target_pose.pose);

    ROS_INFO_STREAM("TARGET POSE FORWARD: " << goal.target_pose.pose);
    ros::Duration(10).sleep();

    geometry_msgs::PoseStamped currentPoseMsg;
    currentPoseMsg.header.frame_id = "/odom";
    currentPoseMsg.header.stamp = ros::Time::now();
    tf::poseTFToMsg(currentPose, currentPoseMsg.pose);

    odomTracker_ = moveBaseClient_->getComponent<OdomTracker>();
    this->odomTracker_->clearPath();
    this->odomTracker_->setStartPoint(currentPoseMsg);
    this->odomTracker_->setWorkingMode(WorkingMode::RECORD_PATH_FORWARD);

    auto plannerSwitcher = moveBaseClient_->getComponent<PlannerSwitcher>();
    plannerSwitcher->setBackwardPlanner();

    moveBaseClient_->sendGoal(goal);
}

void CbNavigateBackwards::onExit()
{
    this->odomTracker_->setWorkingMode(WorkingMode::IDLE);
}

} // namespace cl_move_base_z
