// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/**
 * @ingroup kinematics-dynamics-examples
 * \defgroup cartesianControlExample cartesianControlExample
 *
 * <b>Legal</b>
 *
 * Copyright: (C) 2017 Universidad Carlos III de Madrid;
 *
 * Authors: Raul de Santos Rico
 *
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see license/LGPL.TXT
 *
 * <b>Building</b>
\verbatim
cd examples/cpp/cartesianControlExample/
mkdir build; cd build; cmake ..
make -j$(nproc)
\endverbatim
 * <b>Running example with teoSim</b>
 * First we must run a YARP name server if it is not running in our current namespace:
\verbatim
[on terminal 1] yarp server
\endverbatim
 * What mostly changes is the library command line invocation. We also change the server port name. The following is an example for the simulated robot's right arm.
\verbatim
[on terminal 2] teoSim
[on terminal 3] yarpdev --device BasicCartesianControl --name /teoSim/rightArm/CartesianControl --from /usr/local/share/teo-configuration-files/contexts/kinematics/rightArmKinematics.ini --robot remote_controlboard --local /BasicCartesianControl/teoSim/rightArm --remote /teoSim/rightArm --angleRepr axisAngle
[on terminal 4] ./cartesianControlExample
[on possible terminal 5] yarp rpc /teoSim/rightArm/CartesianControl/rpc_transform:s (for manual operation)
\endverbatim
 */

#include <vector>

#include <yarp/os/LogStream.h>
#include <yarp/os/Network.h>
#include <yarp/os/Property.h>

#include <yarp/dev/PolyDriver.h>

#include <ICartesianControl.h> // we need this to work with the CartesianControlClient device

int main(int argc, char *argv[])
{
    yarp::os::Network yarp;

    if (!yarp::os::Network::checkNetwork())
    {
        yError() << "Please start a yarp name server first";
        return 1;
    }

    yarp::os::Property options;
    options.put("device", "CartesianControlClient"); // our device (a dynamically loaded library)
    options.put("cartesianRemote", "/teoSim/rightArm/CartesianControl"); // remote port through which we'll talk to the server
    options.put("cartesianLocal", "/CartesianControlExample");
    options.put("transform", 1);  // Was yarp::os::Value::getNullValue()

    yarp::dev::PolyDriver dd(options);

    if (!dd.isValid())
    {
        yError() << "Device not available";
        return 1;
    }

    roboticslab::ICartesianControl *iCartesianControl;

    if (!dd.view(iCartesianControl))
    {
        yError() << "Problems acquiring interface";
        return 1;
    }

    yInfo() << "Acquired interface";

    std::vector<double> vector;
    iCartesianControl->stat(vector);

    yInfo() << "Controller status (forward kinematics):" << vector;

    // -- Move arm in joint space:
    // -- set poss (0 0 0 90 0 0 0)

    // -- Position 1
    std::vector<double> position(7);
    position[0] = 0.390926; // 0.390926 -0.346663 0.166873 -0.004334 0.70944 0.704752 0.353119
    position[1] = -0.346663;
    position[2] = 0.166873;
    position[3] = -0.004334;
    position[4] = 0.70944;
    position[5] = 0.704752;
    position[6] = 0.353119;

    // movj -> go to end position in joint space
    // movl -> go to end position in task space

    yInfo() << "Position 1: poss (0 0 0 90 0 0 0)";

    if (iCartesianControl->movj(position))
    {
        iCartesianControl->wait();
    }
    else
    {
        yError() << "failure";
        return 1;
    }

    // -- Position 2: move forward along axis X
    yInfo() << "Position 2: move forward along axis X";
    position[0] = 0.5;

    if (iCartesianControl->movj(position))
    {
        iCartesianControl->wait();
    }
    else
    {
        yError() << "failure";
        return 1;
    }

    // -- Position 3: move right along axis Y
    yInfo() << "Position 3: move right along axis Y";
    position[1] = -0.4;

    if (iCartesianControl->movj(position))
    {
        iCartesianControl->wait();
    }
    else
    {
        yError() << "failure";
        return 1;
    }

    // -- Position 4: rotate -12 degrees about axis Y
    yInfo() << "Position 4: rotate -12 degrees about axis Y ...";
    position[3] = 0.0;
    position[4] = 1.0;
    position[5] = 0.0;
    position[6] = -12.0;

    if (iCartesianControl->movj(position))
    {
        iCartesianControl->wait();
    }
    else
    {
        yError() << "failure";
        return 1;
    }

    // -- Position 5: rotate 50 degrees about axis X
    yInfo() << "Position 5: rotate 50 degrees about axis X ...";
    position[3] = 1.0;
    position[4] = 0.0;
    position[5] = 0.0;
    position[6] = -50.0;

    if (iCartesianControl->movj(position))
    {
        iCartesianControl->wait();
    }
    else
    {
        yError() << "failure";
        return 1;
    }

    // -- Position 6:
    yInfo() << "Position 6: poss (0 0 0 90 0 0 0) ...";
    position[0] = 0.390926; // 0.390926 -0.346663 0.166873 -0.004334 0.70944 0.704752 0.353119
    position[1] = -0.346663;
    position[2] = 0.166873;
    position[3] = -0.004334;
    position[4] = 0.70944;
    position[5] = 0.704752;
    position[6] = 0.353119;

    if (iCartesianControl->movj(position))
    {
        iCartesianControl->wait();
    }
    else
    {
        yError() << "failure";
        return 1;
    }

    // -- Initial position
    yInfo() << "Position 7: Homing ...";
    position[0] = 0.0;
    position[1] = -0.34692;
    position[2] = -0.221806;
    position[3] = 0.0;
    position[4] = 1.0;
    position[5] = 0.0;
    position[6] = 90.0;

    if (iCartesianControl->movj(position))
    {
        iCartesianControl->wait();
    }
    else
    {
        yError() << "failure";
        return 1;
    }

    dd.close();

    return 0;
}
