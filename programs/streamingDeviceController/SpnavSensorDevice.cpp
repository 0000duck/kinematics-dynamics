#include "SpnavSensorDevice.hpp"

#include <yarp/os/LogStream.h>
#include <yarp/sig/Vector.h>

roboticslab::SpnavSensorDevice::SpnavSensorDevice(yarp::os::Searchable & config, bool usingMovi, double gain)
    : StreamingDevice(config),
      iAnalogSensor(NULL),
      usingMovi(usingMovi),
      gain(gain),
      buttonClose(false),
      buttonOpen(false)
{}

bool roboticslab::SpnavSensorDevice::acquireInterfaces()
{
    bool ok = true;

    if (!PolyDriver::view(iAnalogSensor))
    {
        yWarning() << "Could not view iAnalogSensor";
        ok = false;
    }

    return ok;
}

bool roboticslab::SpnavSensorDevice::initialize(bool usingStreamingPreset)
{
    if (usingMovi && gain <= 0.0)
    {
        yWarning() << "Invalid gain for movi command:" << gain;
        return false;
    }

    if (usingStreamingPreset)
    {
        int cmd = usingMovi ? VOCAB_CC_MOVI : VOCAB_CC_TWIST;

        if (!iCartesianControl->setParameter(VOCAB_CC_CONFIG_STREAMING_CMD, cmd))
        {
            yWarning() << "Unable to preset streaming command";
            return false;
        }
    }

    if (!iCartesianControl->setParameter(VOCAB_CC_CONFIG_FRAME, ICartesianSolver::BASE_FRAME))
    {
        yWarning() << "Unable to set inertial reference frame";
        return false;
    }

    if (usingMovi && !iCartesianControl->stat(currentX))
    {
        yWarning() << "Unable to stat initial position";
        return false;
    }

    return true;
}

bool roboticslab::SpnavSensorDevice::acquireData()
{
    yarp::sig::Vector data;
    iAnalogSensor->read(data);

    yDebug() << data.toString(4, 1);

    if (data.size() != 6 && data.size() != 8)
    {
        yWarning() << "Invalid data size:" << data.size();
        return false;
    }

    for (int i = 0; i < 6; i++)
    {
        this->data[i] = data[i];
    }

    if (data.size() == 8)
    {
        buttonClose = data[6] == 1;
        buttonOpen = data[7] == 1;
    }

    return true;
}

bool roboticslab::SpnavSensorDevice::transformData(double scaling)
{
    if (usingMovi)
    {
        for (int i = 0; i < 6; i++)
        {
            if (!fixedAxes[i])
            {
                data[i] = currentX[i] + (gain / scaling) * data[i];
            }
            else
            {
                data[i] = currentX[i];
            }
        }

        return true;
    }
    else
    {
        return StreamingDevice::transformData(scaling);
    }
}

int roboticslab::SpnavSensorDevice::getActuatorState()
{
    if (buttonClose)
    {
        actuatorState = VOCAB_CC_ACTUATOR_CLOSE_GRIPPER;
    }
    else if (buttonOpen)
    {
        actuatorState = VOCAB_CC_ACTUATOR_OPEN_GRIPPER;
    }
    else if (actuatorState != VOCAB_CC_ACTUATOR_NONE)
    {
        if (actuatorState != VOCAB_CC_ACTUATOR_STOP_GRIPPER)
        {
            actuatorState = VOCAB_CC_ACTUATOR_STOP_GRIPPER;
        }
        else
        {
            actuatorState = VOCAB_CC_ACTUATOR_NONE;
        }
    }
    else
    {
        actuatorState = VOCAB_CC_ACTUATOR_NONE;
    }

    return actuatorState;
}

bool roboticslab::SpnavSensorDevice::hasValidMovementData() const
{
    if (usingMovi)
    {
        for (int i = 0; i < 6; i++)
        {
            if (!fixedAxes[i] && data[i] != currentX[i])
            {
                return true;
            }
        }

        return false;
    }
    else
    {
        return StreamingDevice::hasValidMovementData();
    }
}

void roboticslab::SpnavSensorDevice::sendMovementCommand()
{
    if (usingMovi)
    {
        iCartesianControl->movi(data);

        for (int i = 0; i < 6; i++)
        {
            currentX[i] = data[i];
        }
    }
    else
    {
        iCartesianControl->twist(data);
    }
}

void roboticslab::SpnavSensorDevice::stopMotion()
{
    if (!usingMovi)
    {
        std::vector<double> zeros(6, 0.0);
        iCartesianControl->twist(zeros);
    }
}
