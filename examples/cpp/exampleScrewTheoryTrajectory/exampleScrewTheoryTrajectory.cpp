// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/**
 * @ingroup kinematics-dynamics-examples
 * \defgroup screwTheoryTrajectoryExample screwTheoryTrajectoryExample
 *
 * <b>Legal</b>
 *
 * Copyright: (C) 2018 Universidad Carlos III de Madrid;
 *
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see license/LGPL.TXT
 *
 * <b>Building</b>
\verbatim
cd examples/cpp/exampleScrewTheoryTrajectory/
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
[on terminal 3] yarpdev --device BasicCartesianControl --name /teoSim/lefttArm/CartesianControl --from /usr/local/share/teo-configuration-files/contexts/kinematics/lefttArmKinematics.ini --robot remote_controlboard --local /BasicCartesianControl/teoSim/rightArm --remote /teoSim/rightArm --angleRepr axisAngle
[on terminal 4] ./exampleScrewTheoryTrajectory
\endverbatim
 */

#include <memory>
#include <vector>

#include <yarp/os/Bottle.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Network.h>
#include <yarp/os/Property.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/Time.h>

#include <yarp/dev/DeviceDriver.h>
#include <yarp/dev/IControlLimits.h>
#include <yarp/dev/IControlMode.h>
#include <yarp/dev/IEncoders.h>
#include <yarp/dev/IPositionDirect.h>
#include <yarp/dev/PolyDriver.h>

#include <kdl/frames.hpp>

#include <ConfigurationSelector.hpp>
#include <KdlTrajectory.hpp>
#include <KdlVectorConverter.hpp>
#include <KinematicRepresentation.hpp>
#include <MatrixExponential.hpp>
#include <ProductOfExponentials.hpp>
#include <ScrewTheoryIkProblem.hpp>

#include "TrajectoryThread.hpp"

#define DEFAULT_REMOTE_PORT "/teoSim/leftArm"
#define DEFAULT_TRAJ_DURATION 10.0
#define DEFAULT_TRAJ_MAX_VEL 0.05
#define DEFAULT_PERIOD_MS 50.0

namespace rl = roboticslab;

namespace
{
    rl::PoeExpression makeTeoLeftArmKinematics()
    {
        KDL::Frame H_S_0(KDL::Rotation::RotY(-KDL::PI / 2) * KDL::Rotation::RotX(-KDL::PI / 2), KDL::Vector(0, 0.34692, 0.1932 + 0.305));
        KDL::Frame H_0_T(KDL::Vector(-0.63401, 0, 0));

        rl::PoeExpression poe(H_0_T);

        poe.append(rl::MatrixExponential(rl::MatrixExponential::ROTATION, KDL::Vector(0, 0, 1), KDL::Vector::Zero()));
        poe.append(rl::MatrixExponential(rl::MatrixExponential::ROTATION, KDL::Vector(0, 1, 0), KDL::Vector::Zero()));
        poe.append(rl::MatrixExponential(rl::MatrixExponential::ROTATION, KDL::Vector(1, 0, 0), KDL::Vector::Zero()));
        poe.append(rl::MatrixExponential(rl::MatrixExponential::ROTATION, KDL::Vector(0, 0, 1), KDL::Vector(-0.32901, 0, 0)));
        poe.append(rl::MatrixExponential(rl::MatrixExponential::ROTATION, KDL::Vector(1, 0, 0), KDL::Vector(-0.32901, 0, 0)));
        poe.append(rl::MatrixExponential(rl::MatrixExponential::ROTATION, KDL::Vector(0, 0, 1), KDL::Vector(-0.54401, 0, 0)));

        poe.changeBaseFrame(H_S_0);

        return poe;
    }
}

int main(int argc, char *argv[])
{
    yarp::os::Network yarp;

    if (!yarp::os::Network::checkNetwork())
    {
        yError() << "Please start a yarp name server first";
        return 1;
    }

    yarp::os::ResourceFinder rf;
    rf.configure(argc, argv);

    std::string remote = rf.check("remote", yarp::os::Value(DEFAULT_REMOTE_PORT), "remote port").asString();
    double trajDuration = rf.check("trajDuration", yarp::os::Value(DEFAULT_TRAJ_DURATION), "trajectory duration (s)").asFloat64();
    double trajMaxVel = rf.check("trajMaxVel", yarp::os::Value(DEFAULT_TRAJ_MAX_VEL), "trajectory max velocity (m/s)").asFloat64();
    int periodMs = rf.check("periodMs", yarp::os::Value(DEFAULT_PERIOD_MS), "command send period (ms)").asInt32();

    yarp::os::Property jointDeviceOptions;
    jointDeviceOptions.put("device", "remote_controlboard");
    jointDeviceOptions.put("remote", remote);
    jointDeviceOptions.put("local", "/screwTheoryTrajectoryExample" + remote);

    yarp::dev::PolyDriver jointDevice(jointDeviceOptions);

    if (!jointDevice.isValid())
    {
        yError() << "Joint device not available";
        return 1;
    }

    yarp::dev::IEncoders * iEncoders;
    yarp::dev::IControlLimits * iControlLimits;
    yarp::dev::IControlMode * iControlMode;
    yarp::dev::IPositionDirect * iPositionDirect;

    if (!jointDevice.view(iEncoders) || !jointDevice.view(iControlLimits)
            || !jointDevice.view(iControlMode) || !jointDevice.view(iPositionDirect))
    {
        yError() << "Problems acquiring joint interfaces";
        return 1;
    }

    int axes;

    if (!iEncoders->getAxes(&axes))
    {
        yError() << "getAxes() failed";
        return 1;
    }

    std::vector<double> q(axes); // tested at: set poss (0 0 0 -90 0 0)

    while (!iEncoders->getEncoders(q.data()))
    {
        yarp::os::Time::delay(0.1);
    }

    rl::PoeExpression poe = makeTeoLeftArmKinematics();

    axes = poe.size(); // just for real TEO (7 joints, 6 motor axes)

    KDL::JntArray jntArray(axes);

    for (int i = 0; i < axes; i++)
    {
        jntArray(i) = rl::KinRepresentation::degToRad(q[i]);
    }

    KDL::Frame H;

    if (!poe.evaluate(jntArray, H))
    {
        yError() << "FK error";
        return 1;
    }

    rl::ScrewTheoryIkProblemBuilder builder(poe);
    std::auto_ptr<rl::ScrewTheoryIkProblem> ikProblem(builder.build());

    if (!ikProblem.get())
    {
        yError() << "Unable to solve IK";
        return 1;
    }

    KDL::JntArray qMin(axes);
    KDL::JntArray qMax(axes);

    for (int i = 0; i < axes; i++)
    {
        if (!iControlLimits->getLimits(i, &qMin(i), &qMax(i)))
        {
            yError() << "Unable to retrieve limits for joint" << i;
            return 1;
        }
    }

    rl::ConfigurationSelectorLeastOverallAngularDisplacementFactory confFactory(qMin, qMax);
    std::auto_ptr<rl::ConfigurationSelector> ikConfig(confFactory.create());

    std::vector<double> x = rl::KdlVectorConverter::frameToVector(H);

    std::vector<double> xd(x);
    xd[0] += 0.15;
    xd[1] += 0.1;
    xd[2] += 0.1;

    rl::KdlTrajectory trajectory;

    trajectory.setDuration(trajDuration);
    trajectory.setMaxVelocity(trajMaxVel);
    trajectory.addWaypoint(x);
    trajectory.addWaypoint(xd);
    trajectory.configurePath(rl::ICartesianTrajectory::LINE);
    trajectory.configureVelocityProfile(rl::ICartesianTrajectory::TRAPEZOIDAL);

    if (!trajectory.create())
    {
        yError() << "Problem creating cartesian trajectory";
        return 1;
    }

    std::vector<int> modes(axes, VOCAB_CM_POSITION_DIRECT);

    if (!iControlMode->setControlModes(modes.data()))
    {
        yError() << "Unable to change mode";
        return 1;
    }

    TrajectoryThread trajThread(iEncoders, iPositionDirect, ikProblem.get(), ikConfig.get(), &trajectory, periodMs);

    if (trajThread.start())
    {
        yarp::os::Time::delay(trajDuration);
        trajThread.stop();
    }

    jointDevice.close();

    return 0;
}
