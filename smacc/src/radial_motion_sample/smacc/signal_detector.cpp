#include "smacc/signal_detector.h"
#include "smacc/smacc_action_client_base.h"

namespace smacc
{
/**
******************************************************************************************************************
* SignalDetector()
******************************************************************************************************************
*/
SignalDetector::SignalDetector(SmaccScheduler* scheduler)
{
    scheduler_ = scheduler;
}

/**
******************************************************************************************************************
* registerActionClientRequest()
******************************************************************************************************************
*/
void SignalDetector::registerActionClientRequest(ISmaccActionClient* actionClientRequestInfo)
{
    ROS_INFO("Signal detector is aware of the '-- %s -- action client request'", actionClientRequestInfo->getName().c_str());
    openRequests_.push_back(actionClientRequestInfo);
    ROS_INFO("Added to the opened requests list");
}

/**
******************************************************************************************************************
* initialize()
******************************************************************************************************************
*/
void SignalDetector::initialize(ISmaccStateMachine* stateMachine)
{
    smaccStateMachine_ = stateMachine;
}

/**
******************************************************************************************************************
* setProcessorHandle()
******************************************************************************************************************
*/
void SignalDetector::setProcessorHandle(SmaccScheduler::processor_handle processorHandle)
{
    processorHandle_ = processorHandle;
}

/**
******************************************************************************************************************
* runThread()
******************************************************************************************************************
*/
void SignalDetector::runThread()
{
    signalDetectorThread_ = boost::thread( boost::bind(&SignalDetector::pollingLoop, this ));
}

/**
******************************************************************************************************************
* join()
******************************************************************************************************************
*/
void SignalDetector::join()
{
    signalDetectorThread_.join();
}

void SignalDetector::notifyFeedback(ISmaccActionClient* client)
{
    boost::intrusive_ptr< EvActionFeedback > actionFeedbackEvent = new EvActionFeedback();
    actionFeedbackEvent->client = client;

    scheduler_->queue_event(processorHandle_, actionFeedbackEvent);
}

/**
******************************************************************************************************************
* finalizeRequest()
******************************************************************************************************************
*/
void SignalDetector::finalizeRequest(ISmaccActionClient* client)
{
    ROS_INFO("SignalDetector: Finalizing actionlib request: %s. RESULT: %s", client->getName().c_str(), client->getState().toString().c_str());
    auto it = find(openRequests_.begin(),openRequests_.end(),client);

    if (it != openRequests_.end())
    {
        openRequests_.erase(it);
    }

    boost::intrusive_ptr< EvActionResult > actionClientResultEvent = new EvActionResult();
    actionClientResultEvent->client = client;

    ROS_INFO("SignalDetector: Sending successEvent");
    scheduler_->queue_event(processorHandle_, actionClientResultEvent);
}

/**
******************************************************************************************************************
* toString()
******************************************************************************************************************
*/
void SignalDetector::toString(std::stringstream& ss)
{
    ss << "--------" << std::endl;
    ss << "Open requests" << std::endl;
    for(ISmaccActionClient* smaccActionClient: this->openRequests_)
    {
        auto state = smaccActionClient->getState().toString();
        ss << smaccActionClient->getName() << ": " << state << std::endl;
    }
    ss << "--------";
}

/**
******************************************************************************************************************
* poll()
******************************************************************************************************************
*/
void SignalDetector::pollOnce()
{
    for(ISmaccActionClient* smaccActionClient: this->openRequests_)
    {
        // check feedback messages
        while (smaccActionClient->hasFeedback())
        {
            this->notifyFeedback(smaccActionClient);
        }

        // check result
        auto state = smaccActionClient->getState();
        if(state.isDone())
        {
            this->finalizeRequest(smaccActionClient);
        }
    }            
}

/**
******************************************************************************************************************
* pollingLoop()
******************************************************************************************************************
*/
void SignalDetector::pollingLoop()
{
    ros::Rate r(1);
    while (ros::ok())
    {
        this->pollOnce();
        ros::spinOnce();
        r.sleep();
    }
}   
}