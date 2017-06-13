// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Base class for all vehicle subsystems.
//
// =============================================================================

#include "chrono_vehicle/ChPart.h"

#include "chrono_thirdparty/rapidjson/stringbuffer.h"

namespace chrono {
namespace vehicle {

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
ChPart::ChPart(const std::string& name)
    : m_name(name),
      m_friction(0.7f),
      m_restitution(0.1f),
      m_young_modulus(1e7f),
      m_poisson_ratio(0.3f),
      m_kn(2e6),
      m_kt(2e5),
      m_gn(40),
      m_gt(20) {}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChPart::SetContactMaterialProperties(float young_modulus, float poisson_ratio) {
    m_young_modulus = young_modulus;
    m_poisson_ratio = poisson_ratio;
}

void ChPart::SetContactMaterialCoefficients(float kn, float gn, float kt, float gt) {
    m_kn = kn;
    m_gn = gn;
    m_kt = kt;
    m_gt = gt;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChPart::SetVisualizationType(VisualizationType vis) {
    RemoveVisualizationAssets();
    AddVisualizationAssets(vis);
}

// -----------------------------------------------------------------------------
// Utility function for transforming inertia tensors between centroidal frames.
// It converts an inertia matrix specified in a centroidal frame aligned with the
// vehicle reference frame to an inertia matrix expressed in a centroidal body
// reference frame.
// -----------------------------------------------------------------------------
ChMatrix33<> ChPart::TransformInertiaMatrix(
    const ChVector<>& moments,        // moments of inertia in vehicle-aligned centroidal frame
    const ChVector<>& products,       // products of inertia in vehicle-aligned centroidal frame
    const ChMatrix33<>& vehicle_rot,  // vehicle absolute orientation matrix
    const ChMatrix33<>& body_rot      // body absolute orientation matrix
    ) {
    // Calculate rotation matrix body-to-vehicle
    ChMatrix33<> R;
    R.MatrTMultiply(vehicle_rot, body_rot);

    // Assemble the inertia matrix in vehicle-aligned centroidal frame
    ChMatrix33<> J_vehicle(moments, products);

    // Calculate transformed inertia matrix:  (R' * J_vehicle * R)
    ChMatrix33<> tmp;
    tmp.MatrTMultiply(R, J_vehicle);
    return tmp * R;
}

// -----------------------------------------------------------------------------
// Default implementation of the function ExportOutputChannels.
// An override in a derived class must first invoke this method.
// -----------------------------------------------------------------------------
void ChPart::ExportOutputChannels(rapidjson::Document& jsonDocument) const {
    std::string template_name = GetTemplateName();
    jsonDocument.AddMember("name", rapidjson::StringRef(m_name.c_str()), jsonDocument.GetAllocator());
    jsonDocument.AddMember("template", rapidjson::Value(template_name.c_str(), jsonDocument.GetAllocator()).Move(),
                           jsonDocument.GetAllocator());
}

rapidjson::Value ChPart::BodyOutputChannels(std::shared_ptr<ChBody> body,
                                            rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.SetObject();
    obj.AddMember("name", rapidjson::StringRef(body->GetName()), allocator);
    obj.AddMember("position", true, allocator);
    obj.AddMember("velocity", true, allocator);
    obj.AddMember("acceleration", true, allocator);

    return obj;
}

rapidjson::Value ChPart::JointOutputChannels(std::shared_ptr<ChLink> link,
                                             rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.SetObject();
    obj.AddMember("name", rapidjson::StringRef(link->GetName()), allocator);
    obj.AddMember("frame position", true, allocator);
    obj.AddMember("frame orientation", true, allocator);
    obj.AddMember("reaction force", true, allocator);
    obj.AddMember("reaction torque", true, allocator);

    return obj;
}

}  // end namespace vehicle
}  // end namespace chrono
