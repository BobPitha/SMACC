namespace sm_three_some
{
// STATE DECLARATION
struct StState2 : smacc::SmaccState<StState2, MsRun>
{
    using SmaccState::SmaccState;

// DECLARE CUSTOM OBJECT TAGS
    struct TIMEOUT : SUCCESS{};
    struct NEXT : SUCCESS{};
    struct PREVIOUS : ABORT{};

// TRANSITION TABLE
    typedef mpl::list<
   
   Transition<EvTimer<CbTimerCountdownOnce, OrTimer>, StState3, TIMEOUT>,
    Transition<EvAllGo<SrAllEventsGo>, StState3>,
    // Keyboard events
    Transition<EvKeyPressP<CbDefaultKeyboardBehavior, OrKeyboard>, StState1, PREVIOUS>,
    Transition<EvKeyPressN<CbDefaultKeyboardBehavior, OrKeyboard>, StState3, NEXT>
    
    >reactions;

// STATE FUNCTIONS
    static void staticConfigure()
    {
        configure_orthogonal<OrTimer, CbTimerCountdownOnce>(10);
        configure_orthogonal<OrSubscriber, CbWatchdogSubscriberBehavior>();
        configure_orthogonal<OrUpdatablePublisher, CbDefaultPublishLoop>();
        configure_orthogonal<OrKeyboard, CbDefaultKeyboardBehavior>();

        // Create State Reactor
        auto sbAll = static_createStateReactor<SrAllEventsGo>();
        sbAll->addInputEvent<EvKeyPressA<CbDefaultKeyboardBehavior, OrKeyboard>>();
        sbAll->addInputEvent<EvKeyPressB<CbDefaultKeyboardBehavior, OrKeyboard>>();
        sbAll->addInputEvent<EvKeyPressC<CbDefaultKeyboardBehavior, OrKeyboard>>();
        sbAll->setOutputEvent<EvAllGo<SrAllEventsGo>>();
    }

    void runtimeConfigure()
    {
        // get reference to the client
        ClRosTimer *client;
        this->requiresClient(client);

        // subscribe to the timer client callback
        client->onTimerTick(&StState2::onTimerClientTickCallback, this);

        // getting reference to the single countdown behavior
        auto *cbsingle = this->getOrthogonal<OrTimer>()
                             ->getClientBehavior<CbTimerCountdownOnce>();

        // subscribe to the single countdown behavior callback
        cbsingle->onTimerTick(&StState2::onSingleBehaviorTickCallback, this);
    }

    void onEntry()
    {
        ROS_INFO("On Entry!");
    }

    void onExit()
    {
        ROS_INFO("On Exit!");
    }

    void onTimerClientTickCallback()
    {
        ROS_INFO("timer client tick!");
    }

    void onSingleBehaviorTickCallback()
    {
        ROS_INFO("single behavior tick!");
    }
};
} // namespace sm_three_some