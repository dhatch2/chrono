#pragma once
#include "chrono_vehicle/ChDriver.h"
#include "chrono_vehicle/ChVehicle.h"
#include "chrono_vehicle/utils/ChAdaptiveSpeedController.h"
#include "ChDSRCSteeringController.h"

using namespace chrono;
using namespace chrono::vehicle;
//using namespace chrono::vehicle::hmmwv;

namespace chrono {
namespace vehicle {

class CH_VEHICLE_API ChDSRCDriver : public ChDriver {

public:
    ChDSRCDriver(ChVehicle& vehicle, ChVector<> tar, double target_speed, double target_following_time, double target_min_distance, double current_distance);
    ~ChDSRCDriver();

    void SetDesiredSpeed(double val) { m_target_speed = val; }

    void SetDesiredFollowingTime(double val) { m_target_following_time = val; }

    void SetDesiredFollowingMinDistance(double val) { m_target_min_distance = val; }

    void SetCurrentDistance(double val) { m_current_distance = val; }

    void SetThreshholdThrottle(double val) { m_throttle_threshold = val; }

    //void SetTarget(ChVector<>& target);

    void Update(std::string mess);

    std::string GenerateUpdate();

    ChAdaptiveSpeedController& GetSpeedController() { return m_speedPID; }

    ChDSRCSteeringController &GetSteeringController() { return m_steeringPID; }

    void Reset();

    virtual void Advance(double step) override;

    ChDSRCSteeringController m_steeringPID; ///< steering controller
    ChAdaptiveSpeedController m_speedPID;    ///< speed controller

private:
    ChVehicle &vehicle;	//the vehicle being controlled
    ChVector<> target;	//the vehicle being tracked

    double m_target_speed;                   ///< desired vehicle speed
    double m_target_following_time;          ///< desired min following time gap
    double m_target_min_distance;            ///< desired min distance to the vehicle in front
    double m_current_distance;               ///< current distance to the vehicle in front
    double m_throttle_threshold;             ///< throttle value below which brakes are applied

};

}
}
