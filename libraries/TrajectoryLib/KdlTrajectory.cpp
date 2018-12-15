// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "KdlTrajectory.hpp"

#include <limits>

#include <kdl/trajectory_segment.hpp>
#include <kdl/path_line.hpp>
#include <kdl/rotational_interpolation_sa.hpp>
#include <kdl/velocityprofile_trap.hpp>
#include <kdl/velocityprofile_rect.hpp>
#include <kdl/utilities/error.h>

#include <ColorDebug.h>

#include "KdlVectorConverter.hpp"

// -----------------------------------------------------------------------------

roboticslab::KdlTrajectory::KdlTrajectory(double maxVelocity, double maxAcceleration)
    : duration(DURATION_NOT_SET),
      maxVelocity(maxVelocity),
      maxAcceleration(maxAcceleration),
      configuredPath(false),
      configuredVelocityProfile(false),
      velocityDrivenPath(false),
      currentTrajectory(0),
      path(0),
      orient(0),
      velocityProfile(0)
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
    try
    {
        KDL::Twist xdotFrame = currentTrajectory->Vel(movementTime);
        velocity = KdlVectorConverter::twistToVector(xdotFrame);
        return true;
    }
    catch (const KDL::Error_MotionPlanning &e)
    {
        CD_ERROR("Unable to retrieve velocity at %f.\n", movementTime);
        return false;
    }
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::getAcceleration(const double movementTime, std::vector<double>& acceleration)
{
    try
    {
        KDL::Twist xdotdotFrame = currentTrajectory->Acc(movementTime);
        acceleration = KdlVectorConverter::twistToVector(xdotdotFrame);
        return true;
    }
    catch (const KDL::Error_MotionPlanning &e)
    {
        CD_ERROR("Unable to retrieve acceleration at %f.\n", movementTime);
        return false;
    }
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::setDuration(const double duration)
{
    this->duration = duration;
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::addWaypoint(const std::vector<double>& waypoint,
                         const std::vector<double>& waypointVelocity,
                         const std::vector<double>& waypointAcceleration)
{
    KDL::Frame frame = KdlVectorConverter::vectorToFrame(waypoint);
    frames.push_back(frame);

    KDL::Twist twist;

    if ( ! waypointVelocity.empty() )
    {
        twist = KdlVectorConverter::vectorToTwist(waypointVelocity);
    }

    twists.push_back(twist);

    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::configurePath(const int pathType)
{
    switch( pathType )
    {
    case ICartesianTrajectory::LINE:
    {
        if ( frames.empty() || frames.size() > 2 )
        {
            CD_ERROR("Need 2 waypoints (or 1 with initial twist) for Cartesian line (have %d)!\n", frames.size());
            return false;
        }

        orient = new KDL::RotationalInterpolation_SingleAxis();
        double eqradius = 1.0; //0.000001;

        if ( frames.size() == 1 )
        {
            velocityDrivenPath = true;
            path = new KDL::Path_Line(frames[0], twists[0], orient, eqradius);
        }
        else
        {
            velocityDrivenPath = false;
            path = new KDL::Path_Line(frames[0], frames[1], orient, eqradius);
        }

        break;
    }
    default:
        CD_ERROR("Only LINE cartesian path implemented for now!\n");
        return false;
    }

    configuredPath = true;

    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::configureVelocityProfile(const int velocityProfileType)
{
    switch( velocityProfileType )
    {
    case ICartesianTrajectory::TRAPEZOIDAL:
    {
        velocityProfile = new KDL::VelocityProfile_Trap(maxVelocity, maxAcceleration);
        break;
    }
    case ICartesianTrajectory::RECTANGULAR:
    {
        velocityProfile = new KDL::VelocityProfile_Rectangular(maxVelocity);
        break;
    }
    default:
        CD_ERROR("Only TRAPEZOIDAL and RECTANGULAR cartesian velocity profiles implemented for now!\n");
        return false;
    }

    configuredVelocityProfile = true;

    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::create()
{
    if( ! configuredPath )
    {
        CD_ERROR("Path not configured!\n");
        return false;
    }

    if( ! configuredVelocityProfile )
    {
        CD_ERROR("Velocity profile not configured!\n");
        return false;
    }

    if( duration == DURATION_NOT_SET )
    {
        if (velocityDrivenPath)
        {
            // assume we'll execute this trajectory indefinitely; since velocity
            // depends on the path to travel along and the total duration, let's
            // fix both to adjust the resulting velocity as requested by the user

            double vel = path->PathLength(); // distance traveled during 1 time unit
            double dummyGoal = 1e9; // somewhere far away
            double dummyDuration = dummyGoal / vel;

            velocityProfile->SetProfileDuration(0, dummyGoal, dummyDuration);
            currentTrajectory = new KDL::Trajectory_Segment(path, velocityProfile);
        }
        else
        {
            velocityProfile->SetProfile(0, path->PathLength());
            currentTrajectory = new KDL::Trajectory_Segment(path, velocityProfile);
        }
    }
    else
    {
        if (velocityDrivenPath)
        {
            // execute the trajectory given an initial velocity and duration

            double vel = path->PathLength(); // distance traveled during 1 time unit
            double guessedGoal = vel * duration;

            velocityProfile->SetProfileDuration(0, guessedGoal, duration);
            currentTrajectory = new KDL::Trajectory_Segment(path, velocityProfile);
        }
        else
        {
            // velocity profile is set under the hood
            currentTrajectory = new KDL::Trajectory_Segment(path, velocityProfile, duration);
        }
    }

    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlTrajectory::destroy()
{
    delete currentTrajectory; // deletes aggregated path and profile instances, too
    currentTrajectory = 0;
    path = 0;
    orient = 0;
    velocityProfile = 0;

    duration = DURATION_NOT_SET;
    configuredPath = configuredVelocityProfile = false;
    velocityDrivenPath = false;

    frames.clear();
    twists.clear();

    return true;
}

// -----------------------------------------------------------------------------
