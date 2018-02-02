// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#ifndef __ASIBOT_SOLVER_HPP__
#define __ASIBOT_SOLVER_HPP__

#include <string>
#include <vector>

#include <yarp/os/Searchable.h>
#include <yarp/dev/DeviceDriver.h>

#include "AsibotConfiguration.hpp"
#include "ICartesianSolver.h"

#define NUM_MOTORS 5

#define DEFAULT_A0 0.3
#define DEFAULT_A1 0.4
#define DEFAULT_A2 0.4
#define DEFAULT_A3 0.3

#define DEFAULT_STRATEGY "leastOverallAngularDisplacement"

namespace roboticslab
{

/**
 * @ingroup YarpPlugins
 * @defgroup AsibotSolver
 *
 * @brief Contains roboticslab::AsibotSolver.
 */

/**
 * @ingroup AsibotSolver
 * @brief The AsibotSolver implements ICartesianSolver.
 */

class AsibotSolver : public yarp::dev::DeviceDriver, public ICartesianSolver
{
public:

    AsibotSolver()
        : A0(DEFAULT_A0), A1(DEFAULT_A1), A2(DEFAULT_A2), A3(DEFAULT_A3),
          confFactory(NULL)
    {}

// -------- ICartesianSolver declarations. Implementation in ICartesianSolverImpl.cpp --------

    // Get number of joints for which the solver has been configured.
    virtual bool getNumJoints(int* numJoints);

    // Append an additional link.
    virtual bool appendLink(const std::vector<double> &x);

    // Restore original kinematic chain.
    virtual bool restoreOriginalChain();

    // Perform forward kinematics.
    virtual bool fwdKin(const std::vector<double> &q, std::vector<double> &x);

    // Obtain error with respect to forward kinematics.
    virtual bool fwdKinError(const std::vector<double> &xd, const std::vector<double> &q, std::vector<double> &x, reference_frame frame);

    // Perform inverse kinematics.
    virtual bool invKin(const std::vector<double> &xd, const std::vector<double> &qGuess, std::vector<double> &q, reference_frame frame);

    // Perform differential inverse kinematics.
    virtual bool diffInvKin(const std::vector<double> &q, const std::vector<double> &xdot, std::vector<double> &qdot, reference_frame frame);

    // Perform inverse dynamics.
    virtual bool invDyn(const std::vector<double> &q, std::vector<double> &t);

    // Perform inverse dynamics.
    virtual bool invDyn(const std::vector<double> &q,const std::vector<double> &qdot,const std::vector<double> &qdotdot, const std::vector< std::vector<double> > &fexts, std::vector<double> &t);

    // Set joint limits.
    virtual bool setLimits(const std::vector<double> &qMin, const std::vector<double> &qMax);

// -------- DeviceDriver declarations. Implementation in IDeviceImpl.cpp --------

    /**
    * Open the DeviceDriver. 
    * @param config is a list of parameters for the device.
    * Which parameters are effective for your device can vary.
    * See \ref dev_examples "device invocation examples".
    * If there is no example for your device,
    * you can run the "yarpdev" program with the verbose flag
    * set to probe what parameters the device is checking.
    * If that fails too,
    * you'll need to read the source code (please nag one of the 
    * yarp developers to add documentation for your device).
    * @return true/false upon success/failure
    */
    virtual bool open(yarp::os::Searchable& config);

    /**
    * Close the DeviceDriver.
    * @return true/false on success/failure.
    */
    virtual bool close();

private:

    // defined in DeviceDriverImpl.cpp
    bool buildStrategyFactory(const std::string & strategy);

    AsibotConfiguration * getConfiguration() const
    {
        return confFactory->create();
    }

    double A0, A1, A2, A3;  // link lengths

    std::vector<double> qMin, qMax;

    AsibotConfigurationFactory * confFactory;
};

}  // namespace roboticslab

#endif  // __ASIBOT_SOLVER_HPP__
