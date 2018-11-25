// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "MatrixExponential.hpp"

#include <ColorDebug.h>

#include "ScrewTheoryTools.hpp"

using namespace roboticslab;

// -----------------------------------------------------------------------------

namespace
{
    inline KDL::Rotation operator-(const KDL::Rotation & lhs, const KDL::Rotation & rhs)
    {
        return KDL::Rotation(lhs(0, 0) - rhs(0, 0), lhs(0, 1) - rhs(0, 1), lhs(0, 2) - rhs(0, 2),
                             lhs(1, 0) - rhs(1, 0), lhs(1, 1) - rhs(1, 1), lhs(1, 2) - rhs(1, 2),
                             lhs(2, 0) - rhs(2, 0), lhs(2, 1) - rhs(2, 1), lhs(2, 2) - rhs(2, 2));
    }
}

// -----------------------------------------------------------------------------

MatrixExponential::MatrixExponential(motion _motionType, const KDL::Vector & _axis, const KDL::Vector & _origin)
    : motionType(_motionType),
      axis(_axis),
      origin(_origin)
{
    axis.Normalize();
}

// -----------------------------------------------------------------------------

KDL::Frame MatrixExponential::asFrame(double theta) const
{
    KDL::Frame H = KDL::Frame::Identity();

    if (motionType == ROTATION)
    {
        KDL::Vector v = - axis * origin;
        H.M = KDL::Rotation::Rot2(axis, theta);
        H.p = (KDL::Rotation::Identity() - H.M) * (axis * v) + (vectorPow2(axis) * v * theta);
    }
    else if (motionType == TRANSLATION)
    {
        H.p = axis * theta;
    }
    else
    {
        CD_WARNING("Unrecognized motion type: %d.\n", motionType);
    }

    return H;
}

// -----------------------------------------------------------------------------
