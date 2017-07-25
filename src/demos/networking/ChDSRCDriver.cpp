#include "ChDSRCDriver.h"
#include "chrono_vehicle/ChVehicle.h"
#include "MessageConversions.h"

ChDSRCDriver::ChDSRCDriver(ChVehicle& veh,
    ChVector<> tar,
    double target_speed,
    double target_following_time,
    double target_min_distance,
    double current_distance) : ChDriver(veh),
	m_target_speed(target_speed),
	m_target_following_time(target_following_time),
	m_target_min_distance(target_min_distance),
	m_current_distance(current_distance),
	m_throttle_threshold(0.2),
	vehicle(veh),
	target(tar),
	m_steeringPID(tar) {
    //default behavior; can be overridden later with m_xxxxxPID.SetGains(Kp,Ki,Kd)
	m_steeringPID.SetGains(0.1, 0.0, 0.02);
	m_speedPID.SetGains(1.0, 0.2, 0.2);
	//init everything to zero
	SetSteering(0);
	m_speedPID.Reset(m_vehicle);
	m_steeringPID.Reset(m_vehicle);
}

ChDSRCDriver::~ChDSRCDriver() {
}

void ChDSRCDriver::Reset()
{
	SetSteering(0);
	m_speedPID.Reset(m_vehicle);
	m_steeringPID.Reset(m_vehicle);
}

void ChDSRCDriver::Update(std::string mess) {
    ChronoMessages::PositionUpdate update;
    update.ParseFromString(mess);
    if (update.idnumber() == 0) {
        target.Set(update.position().x(), update.position().y(), update.position().z());
    }
}

std::string ChDSRCDriver::GenerateUpdate() {
    std::string update;
    ChronoMessages::PositionUpdate message;
    message.set_idnumber(0);
    messageFromVector(message.mutable_position(), vehicle.GetDriverPos());
    message.SerializeToString(&update);
    return update;
}

void ChDSRCDriver::Advance(double step) {
    // TODO: Fix this so that is turns towards the target vehicle
    ChVector<> selfPos = vehicle.GetDriverPos();

    m_current_distance = (target - selfPos).Length();

    double out_speed = m_speedPID.Advance(m_vehicle, m_target_speed, m_target_following_time, m_target_min_distance, m_current_distance, step);
    ChClampValue(out_speed, -1.0, 1.0);

    if (out_speed > 0) {
        // Vehicle moving too slow
        m_braking = 0;
        m_throttle = out_speed;
    }
    else if (m_throttle > m_throttle_threshold) {
        // Vehicle moving too fast: reduce throttle
        m_braking = 0;
        m_throttle = 1 + out_speed;
    }
    else {
        // Vehicle moving too fast: apply brakes
        m_braking = -out_speed;
        m_throttle = 0;
    }

}
