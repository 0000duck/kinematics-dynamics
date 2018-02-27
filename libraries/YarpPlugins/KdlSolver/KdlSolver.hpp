// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#ifndef __KDL_SOLVER_HPP__
#define __KDL_SOLVER_HPP__

#include <string>

#include <yarp/os/Searchable.h>
#include <yarp/dev/DeviceDriver.h>
#include <yarp/sig/Matrix.h>

#include "ICartesianSolver.h"

#define DEFAULT_KINEMATICS "none.ini"  // string
#define DEFAULT_NUM_LINKS 1  // int

#define DEFAULT_EPSILON 0.005     // Precision tolerance
#define DEFAULT_DURATION 20     // For Trajectory
#define DEFAULT_MAXVEL 7.5      // unit/s
#define DEFAULT_MAXACC 0.2      // unit/s^2

#define DEFAULT_EPS 1e-9
#define DEFAULT_MAXITER 1000

namespace roboticslab
{

/**
 * @ingroup YarpPlugins
 * \defgroup KdlSolver
 *
 * @brief Contains roboticslab::KdlSolver.
 */

class KdlSolverImpl;

/**
 * @ingroup KdlSolver
 * @brief Proxy class for the real implementation, KdlSolverImpl.
 */
class KdlSolver : public yarp::dev::DeviceDriver, public ICartesianSolver
{
public:

    KdlSolver();

    // -- ICartesianSolver declarations --

    // Get number of joints for which the solver has been configured.
    virtual bool getNumJoints(int* numJoints);

    // Append an additional link.
    virtual bool appendLink(const std::vector<double>& x);

    // Restore original kinematic chain.
    virtual bool restoreOriginalChain();

    // Change reference frame.
    virtual bool changeOrigin(const std::vector<double> &x_old_obj, const std::vector<double> &x_new_old, std::vector<double> &x_new_obj);

    // Perform forward kinematics.
    virtual bool fwdKin(const std::vector<double> &q, std::vector<double> &x);

    // Obtain difference between supplied pose inputs.
    virtual bool poseDiff(const std::vector<double> &xLhs, const std::vector<double> &xRhs, std::vector<double> &xOut);

    // Perform inverse kinematics.
    virtual bool invKin(const std::vector<double> &xd, const std::vector<double> &qGuess, std::vector<double> &q, const reference_frame frame);

    // Perform differential inverse kinematics.
    virtual bool diffInvKin(const std::vector<double> &q, const std::vector<double> &xdot, std::vector<double> &qdot, const reference_frame frame);

    // Perform inverse dynamics.
    virtual bool invDyn(const std::vector<double> &q, std::vector<double> &t);

    // Perform inverse dynamics.
    virtual bool invDyn(const std::vector<double> &q, const std::vector<double> &qdot, const std::vector<double> &qdotdot, const std::vector< std::vector<double> > &fexts, std::vector<double> &t);

    // -------- DeviceDriver declarations --------

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

    bool getMatrixFromProperties(yarp::os::Searchable &options, std::string &tag, yarp::sig::Matrix &H);

    KdlSolverImpl * impl;
};

}  // namespace roboticslab

#endif  // __KDL_SOLVER_HPP__
