// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "BasicTwoLimbCartesianControl.hpp"

// ------------------- ICartesianControl Related ------------------------------------

bool teo::BasicTwoLimbCartesianControl::stat(int &state, std::vector<double> &x)
{
    std::vector<double> currentQ(numRobotJoints);
    if ( ! iEncoders->getEncoders( currentQ.data() ) )
    {
        CD_ERROR("getEncoders failed.\n");
        return false;
    }
    if ( ! iCartesianSolver->fwdKin(currentQ,x) )
    {
        CD_ERROR("fwdKin failed.\n");
        return false;
    }
    state = getCurrentState();
    return true;
}

// -----------------------------------------------------------------------------

bool teo::BasicTwoLimbCartesianControl::step(const std::vector<double> &xd)
{
    CD_WARNING("MOVL mode still experimental.\n");

    std::vector<double> currentQ(numRobotJoints), x;
    if ( ! iEncoders->getEncoders( currentQ.data() ) )
    {
        CD_ERROR("getEncoders failed.\n");
        return false;
    }
    if ( ! iCartesianSolver->fwdKin(currentQ,x) )
    {
        CD_ERROR("fwdKin failed.\n");
        return false;
    }
    trajectory.newLine(x,xd);

    //-- Set velocity mode and set state which makes rate thread implement control.
    iVelocityControl->setVelocityMode();
    movementStartTime = yarp::os::Time::now();
    setCurrentState( VOCAB_CC_MOVS_CONTROLLING );

    //-- Wait for movement to be done, then delete
    CD_SUCCESS("Waiting\n");
    while( getCurrentState() == VOCAB_CC_MOVS_CONTROLLING )
    {
        printf(".");
        fflush(stdout);
        yarp::os::Time::delay(0.5);
    }
    //trajectory.deleteLine();  //-- Causes segFaults for now

    return true;
}

// -----------------------------------------------------------------------------

bool teo::BasicTwoLimbCartesianControl::stopControl()
{
    iPositionControl->setPositionMode();
    iPositionControl->stop();
    setCurrentState( VOCAB_CC_NOT_CONTROLLING );
    return true;
}

// -----------------------------------------------------------------------------
