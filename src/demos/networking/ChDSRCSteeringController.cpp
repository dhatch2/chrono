#include "ChDSRCSteeringController.h"

namespace chrono {
namespace vehicle {

ChDSRCSteeringController::ChDSRCSteeringController(ChVector<>& tar) : target(tar) {
}

ChDSRCSteeringController::~ChDSRCSteeringController(){
}

void ChDSRCSteeringController::CalcTargetLocation() {
	m_target = target;
}

void ChDSRCSteeringController::SetTarget(ChVector<>& tar) {
	target = tar;
}

}
}
