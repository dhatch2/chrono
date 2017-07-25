#pragma once
#include "chrono_vehicle/utils/ChSteeringController.h"
#include "chrono/core/ChMathematics.h"

namespace chrono {
namespace vehicle {

class CH_VEHICLE_API ChDSRCSteeringController : public ChSteeringController {
public:
	ChDSRCSteeringController(ChVector<> tar);
	~ChDSRCSteeringController();
	void SetTarget(ChVector<> tar);

private:
	ChVector<> target;
	void CalcTargetLocation();
};

}
}
