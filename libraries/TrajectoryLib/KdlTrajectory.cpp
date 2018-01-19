// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "KdlTrajectory.hpp"

#include <kdl/frames.hpp>
#include <kdl/velocityprofile_trap.hpp>
#include <kdl/path_line.hpp>

#include <ColorDebug.hpp>

#include "KdlVectorConverter.hpp"

// -----------------------------------------------------------------------------

roboticslab::KdlTrajectory::KdlTrajectory()
    : currentTrajectory(0),
      _orient(0),
      _duration(DURATION_NOT_SET),
      configuredPath(false)
{}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::getDuration(double* duration) const
{
    *duration = currentTrajectory->Duration();
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::getPosition(const double movementTime, std::vector<double>& position)
{
    KDL::Frame xFrame = currentTrajectory->Pos(movementTime);
    position = KdlVectorConverter::frameToVector(xFrame);
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::getVelocity(const double movementTime, std::vector<double>& velocity)
{
    KDL::Twist xdotFrame = currentTrajectory->Vel(movementTime);
    velocity = KdlVectorConverter::twistToVector(xdotFrame);
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::getAcceleration(const double movementTime, std::vector<double>& acceleration)
{
    KDL::Twist xdotdotFrame = currentTrajectory->Acc(movementTime);
    acceleration = KdlVectorConverter::twistToVector(xdotdotFrame);
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::setDuration(const double duration)
{
    _duration = duration;
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::addWaypoint(const std::vector<double>& waypoint,
                         const std::vector<double>& waypointVelocity,
                         const std::vector<double>& waypointAcceleration)
{
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::configurePath(const std::vector<double> &src, const std::vector<double> &dest)
{
    KDL::Frame srcFrame = KdlVectorConverter::vectorToFrame(src);
    KDL::Frame destFrame = KdlVectorConverter::vectorToFrame(dest);

    double _eqradius = 1.0; //0.000001;
    KDL::Path * path = new KDL::Path_Line(srcFrame, destFrame, _orient, _eqradius);

    configuredPath = true;
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::create()
{
    if( DURATION_NOT_SET == _duration )
    {
        CD_ERROR("Duration not set!");
        return false;
    }
    if( ! configuredPath )
    {
        CD_ERROR("Path not configured!");
        return false;
    }

    _orient = new KDL::RotationalInterpolation_SingleAxis();

    KDL::VelocityProfile * velocityProfile = new KDL::VelocityProfile_Trap(DEFAULT_CARTESIAN_MAX_VEL, DEFAULT_CARTESIAN_MAX_ACC);

    currentTrajectory = new KDL::Trajectory_Segment(path, velocityProfile, _duration);

    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::destroy()
{
    delete currentTrajectory;  // deletes _orient, too
    currentTrajectory = 0;
    _orient = 0;

    return true;
}

// -----------------------------------------------------------------------------
