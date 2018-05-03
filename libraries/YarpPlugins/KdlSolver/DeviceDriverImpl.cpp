// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include "KdlSolver.hpp"

#include <kdl/segment.hpp>
#include <kdl/rigidbodyinertia.hpp>
#include <kdl/rotationalinertia.hpp>

#include <ColorDebug.hpp>

#include "KinematicRepresentation.hpp"

// ------------------- DeviceDriver Related ------------------------------------

bool roboticslab::KdlSolver::open(yarp::os::Searchable& config)
{

    CD_DEBUG("config: %s.\n", config.toString().c_str());

    //-- kinematics
    std::string kinematics = config.check("kinematics",yarp::os::Value(DEFAULT_KINEMATICS),"path to file with description of robot kinematics").asString();
    CD_INFO("kinematics: %s [%s]\n", kinematics.c_str(),DEFAULT_KINEMATICS);
    yarp::os::ResourceFinder rf;
    rf.setVerbose(false);
    rf.setDefaultContext("kinematics");
    std::string kinematicsFullPath = rf.findFileByName(kinematics);

    yarp::os::Property fullConfig;
    fullConfig.fromConfigFile(kinematicsFullPath.c_str());
    fullConfig.fromString(config.toString(),false);  //-- Can override kinematics file contents.
    fullConfig.setMonitor(config.getMonitor(), "KdlSolver");

    CD_DEBUG("fullConfig: %s.\n", fullConfig.toString().c_str());

    //-- numlinks
    int numLinks = fullConfig.check("numLinks",yarp::os::Value(DEFAULT_NUM_LINKS),"chain number of segments").asInt();
    CD_INFO("numLinks: %d [%d]\n",numLinks,DEFAULT_NUM_LINKS);

    //-- gravity
    yarp::os::Value defaultGravityValue;
    yarp::os::Bottle *defaultGravityBottle = defaultGravityValue.asList();
    defaultGravityBottle->addDouble(0);
    defaultGravityBottle->addDouble(0);
    defaultGravityBottle->addDouble(-9.81);

    yarp::os::Value gravityValue = fullConfig.check("gravity", defaultGravityValue, "gravity vector (SI units)");
    yarp::os::Bottle *gravityBottle = gravityValue.asList();
    gravity = KDL::Vector(gravityBottle->get(0).asDouble(),gravityBottle->get(1).asDouble(),gravityBottle->get(2).asDouble());
    CD_INFO("gravity: %s [%s]\n",gravityBottle->toString().c_str(),defaultGravityBottle->toString().c_str());

    //-- H0
    yarp::sig::Matrix defaultYmH0(4,4);
    defaultYmH0.eye();

    yarp::sig::Matrix ymH0(4,4);
    std::string ymH0_str("H0");
    if( ! getMatrixFromProperties(fullConfig, ymH0_str, ymH0))
    {
        ymH0 = defaultYmH0;
    }

    KDL::Vector kdlVec0(ymH0(0,3),ymH0(1,3),ymH0(2,3));
    KDL::Rotation kdlRot0( ymH0(0,0),ymH0(0,1),ymH0(0,2),ymH0(1,0),ymH0(1,1),ymH0(1,2),ymH0(2,0),ymH0(2,1),ymH0(2,2));
    chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::None), KDL::Frame(kdlRot0,kdlVec0)));  //-- H0 = Frame(kdlRot0,kdlVec0);
    CD_INFO("H0:\n%s\n[%s]\n",ymH0.toString().c_str(),defaultYmH0.toString().c_str());

    //-- links
    for(int linkIndex=0;linkIndex<numLinks;linkIndex++)
    {
        std::string link("link_");
        std::ostringstream s;
        s << linkIndex;
        link += s.str();
        yarp::os::Bottle &bLink = fullConfig.findGroup(link);
        if( ! bLink.isNull() ) {
            //-- Kinematic
            double linkOffset = bLink.check("offset",yarp::os::Value(0.0), "DH joint angle (degrees)").asDouble();
            double linkD = bLink.check("D",yarp::os::Value(0.0), "DH link offset (meters)").asDouble();
            double linkA = bLink.check("A",yarp::os::Value(0.0), "DH link length (meters)").asDouble();
            double linkAlpha = bLink.check("alpha",yarp::os::Value(0.0), "DH link twist (degrees)").asDouble();
            //-- Dynamic
            if( bLink.check("mass") && bLink.check("cog") && bLink.check("inertia")) {
                double linkMass = bLink.check("mass",yarp::os::Value(0.0), "link mass (SI units)").asDouble();
                yarp::os::Bottle linkCog = bLink.findGroup("cog", "vector of link's center of gravity (SI units)").tail();
                yarp::os::Bottle linkInertia = bLink.findGroup("inertia", "vector of link's inertia (SI units)").tail();
                chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::RotZ), KDL::Frame::DH(linkA,KinRepresentation::degToRad(linkAlpha),linkD,KinRepresentation::degToRad(linkOffset)),
                                              KDL::RigidBodyInertia(linkMass,KDL::Vector(linkCog.get(0).asDouble(),linkCog.get(1).asDouble(),linkCog.get(2).asDouble()),
                                                                    KDL::RotationalInertia(linkInertia.get(0).asDouble(),linkInertia.get(1).asDouble(),linkInertia.get(2).asDouble(),0,0,0))));
                CD_INFO("Added: %s (offset %f) (D %f) (A %f) (alpha %f) (mass %f) (cog %f %f %f) (inertia %f %f %f)\n",
                           link.c_str(), linkOffset,linkD,linkA,linkAlpha,linkMass,
                           linkCog.get(0).asDouble(),linkCog.get(1).asDouble(),linkCog.get(2).asDouble(),
                           linkInertia.get(0).asDouble(),linkInertia.get(1).asDouble(),linkInertia.get(2).asDouble());
            }
            else //-- No mass -> skip dynamics
            {
                chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::RotZ),KDL::Frame::DH(linkA,KinRepresentation::degToRad(linkAlpha),linkD,KinRepresentation::degToRad(linkOffset))));
                CD_INFO("Added: %s (offset %f) (D %f) (A %f) (alpha %f)\n",link.c_str(), linkOffset,linkD,linkA,linkAlpha);
            }
            continue;
        }

        std::string xyzLink("xyzLink_");
        std::ostringstream xyzS;
        xyzS << linkIndex;
        xyzLink += xyzS.str();
        CD_WARNING("Not found: \"%s\", looking for \"%s\" instead.\n", link.c_str(), xyzLink.c_str());
        yarp::os::Bottle &bXyzLink = fullConfig.findGroup(xyzLink);
        if( bXyzLink.isNull() ) {
            CD_ERROR("Not found: \"%s\" either.\n", xyzLink.c_str());
            return false;
        }
        double linkX = bXyzLink.check("x",yarp::os::Value(0.0), "X coordinate of next frame (meters)").asDouble();
        double linkY = bXyzLink.check("y",yarp::os::Value(0.0), "Y coordinate of next frame (meters)").asDouble();
        double linkZ = bXyzLink.check("z",yarp::os::Value(0.0), "Z coordinate of next frame (meters)").asDouble();

        std::string linkTypes = "joint type (Rot[XYZ]|InvRot[XYZ]|Trans[XYZ]|InvTrans[XYZ]), e.g. 'RotZ'";
        std::string linkType = bXyzLink.check("Type",yarp::os::Value("NULL"), linkTypes.c_str()).asString();
        if(linkType == "RotX") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::RotX),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "RotY") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::RotY),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "RotZ") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::RotZ),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "InvRotX") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::RotX,-1.0),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "InvRotY") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::RotY,-1.0),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "InvRotZ") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::RotZ,-1.0),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "TransX") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::TransX),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "TransY") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::TransY),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "TransZ") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::TransZ),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "InvTransX") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::TransX,-1.0),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "InvTransY") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::TransY,-1.0),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else if(linkType == "InvTransZ") {
            chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::TransZ,-1.0),KDL::Frame(KDL::Vector(linkX,linkY,linkZ))));
        } else {
            CD_WARNING("Link joint type \"%s\" unrecognized!\n",linkType.c_str());
        }

        CD_SUCCESS("Added: %s (Type %s) (x %f) (y %f) (z %f)\n",xyzLink.c_str(),linkType.c_str(),linkX,linkY,linkZ);
    }

    //-- HN
    yarp::sig::Matrix defaultYmHN(4,4);
    defaultYmHN.eye();

    yarp::sig::Matrix ymHN(4,4);
    std::string ymHN_str("HN");
    if( ! getMatrixFromProperties(fullConfig, ymHN_str, ymHN))
    {
        ymHN = defaultYmHN;
    }

    KDL::Vector kdlVecN(ymHN(0,3),ymHN(1,3),ymHN(2,3));
    KDL::Rotation kdlRotN( ymHN(0,0),ymHN(0,1),ymHN(0,2),ymHN(1,0),ymHN(1,1),ymHN(1,2),ymHN(2,0),ymHN(2,1),ymHN(2,2));
    chain.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::None), KDL::Frame(kdlRotN,kdlVecN)));
    CD_INFO("HN:\n%s\n[%s]\n",ymHN.toString().c_str(),defaultYmHN.toString().c_str());

    //--
    CD_INFO("Chain number of segments (post- H0 and HN): %d\n",chain.getNrOfSegments());
    CD_INFO("Chain number of joints (post- H0 and HN): %d\n",chain.getNrOfJoints());

    qMax.resize(chain.getNrOfJoints());
    qMin.resize(chain.getNrOfJoints());

    //-- Joint limits
    if (!fullConfig.check("mins") || !fullConfig.check("maxs"))
    {
        CD_ERROR("Missing 'mins' and/or 'maxs' option(s).\n");
        return false;
    }

    yarp::os::Bottle *maxs = fullConfig.findGroup("maxs", "joint upper limits (meters or degrees)").get(1).asList();
    yarp::os::Bottle *mins = fullConfig.findGroup("mins", "joint lower limits (meters or degrees)").get(1).asList();

    if (maxs == YARP_NULLPTR || mins == YARP_NULLPTR)
    {
        CD_ERROR("Empty 'mins' and/or 'maxs' option(s)\n");
        return false;
    }

    if (maxs->size() != chain.getNrOfJoints() || mins->size() != chain.getNrOfJoints())
    {
        CD_ERROR("chain.getNrOfJoints (%d) != maxs.size(), mins.size() (%d, %d)\n", chain.getNrOfJoints(), maxs->size(), mins->size());
        return false;
    }

    for (int motor=0; motor<chain.getNrOfJoints(); motor++)
    {
        qMax(motor) = maxs->get(motor).asDouble();
        qMin(motor) = mins->get(motor).asDouble();

        if (qMin(motor) == qMax(motor))
        {
            CD_WARNING("qMin == qMax (%f) at joint %d\n", qMin(motor), motor);
        }
        else if (qMin(motor) > qMax(motor))
        {
            CD_ERROR("qMin > qMax (%f > %f) at joint %d\n", qMin(motor), qMax(motor), motor);
            return false;
        }
    }

    //-- Precision and max iterations for IK solver.
    eps = fullConfig.check("eps", yarp::os::Value(DEFAULT_EPS), "IK solver precision (meters)").asDouble();
    maxIter = fullConfig.check("maxIter", yarp::os::Value(DEFAULT_MAXITER), "maximum number of iterations").asInt();

    originalChain = chain;  // We have: Chain& operator = (const Chain& arg);

    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlSolver::close() {
    return true;
}

// -----------------------------------------------------------------------------

bool roboticslab::KdlSolver::getMatrixFromProperties(yarp::os::Searchable &options, std::string &tag, yarp::sig::Matrix &H) {

    yarp::os::Bottle *bH=options.find(tag).asList();
    if (!bH) return false;

    int i=0;
    int j=0;
    H.zero();
    for (int cnt=0; (cnt<bH->size()) && (cnt<H.rows()*H.cols()); cnt++) {
        H(i,j)=bH->get(cnt).asDouble();
        if (++j>=H.cols()) {
            i++;
            j=0;
        }
    }
    return true;
}

// -----------------------------------------------------------------------------

KDL::Chain roboticslab::KdlSolver::getChain() const
{
    KDL::Chain localChain;
    mutex.wait();
    localChain = chain;
    mutex.post();
    return localChain;
}

// -----------------------------------------------------------------------------

void roboticslab::KdlSolver::setChain(const KDL::Chain & chain)
{
    mutex.wait();
    this->chain = chain;
    mutex.post();
}

// -----------------------------------------------------------------------------
