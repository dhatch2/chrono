// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
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
// Derived classes should override this function and first invoke this base
// class implementation, followed by calls to the various static
// Export***OutputChannels functions, as appropriate.
// -----------------------------------------------------------------------------
void ChPart::ExportOutputChannels(rapidjson::Document& jsonDocument) const {
    std::string template_name = GetTemplateName();
    jsonDocument.AddMember("name", rapidjson::StringRef(m_name.c_str()), jsonDocument.GetAllocator());
    jsonDocument.AddMember("template", rapidjson::Value(template_name.c_str(), jsonDocument.GetAllocator()).Move(),
                           jsonDocument.GetAllocator());
}

void ChPart::ExportBodyOutputChannels(rapidjson::Document& jsonDocument, std::vector<std::shared_ptr<ChBody>> bodies) {
    rapidjson::Document::AllocatorType& allocator = jsonDocument.GetAllocator();

    rapidjson::Value jsonArray(rapidjson::kArrayType);
    for (auto body : bodies) {
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.SetObject();
        obj.AddMember("name", rapidjson::StringRef(body->GetName()), allocator);
        obj.AddMember("position", true, allocator);
        obj.AddMember("velocity", true, allocator);
        obj.AddMember("acceleration", true, allocator);
        jsonArray.PushBack(obj, allocator);
    }
    jsonDocument.AddMember("bodies", jsonArray, allocator);
}

void ChPart::ExportShaftOutputChannels(rapidjson::Document& jsonDocument,
                                       std::vector<std::shared_ptr<ChShaft>> shafts) {
    rapidjson::Document::AllocatorType& allocator = jsonDocument.GetAllocator();

    rapidjson::Value jsonArray(rapidjson::kArrayType);
    for (auto shaft : shafts) {
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.SetObject();
        obj.AddMember("name", rapidjson::StringRef(shaft->GetName()), allocator);
        obj.AddMember("angular velocity", true, allocator);
        obj.AddMember("reaction torque", true, allocator);
        jsonArray.PushBack(obj, allocator);
    }
    jsonDocument.AddMember("shafts", jsonArray, allocator);
}

void ChPart::ExportJointOutputChannels(rapidjson::Document& jsonDocument, std::vector<std::shared_ptr<ChLink>> joints) {
    rapidjson::Document::AllocatorType& allocator = jsonDocument.GetAllocator();

    rapidjson::Value jsonArray(rapidjson::kArrayType);
    for (auto joint : joints) {
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.SetObject();
        obj.AddMember("name", rapidjson::StringRef(joint->GetName()), allocator);
        obj.AddMember("frame position", true, allocator);
        obj.AddMember("frame orientation", true, allocator);
        obj.AddMember("reaction force", true, allocator);
        obj.AddMember("reaction torque", true, allocator);
        obj.AddMember("constraint violation", true, allocator);
        jsonArray.PushBack(obj, allocator);
    }
    jsonDocument.AddMember("joints", jsonArray, allocator);
}

void ChPart::ExportMarkerOutputChannels(rapidjson::Document& jsonDocument,
                                        std::vector<std::shared_ptr<ChMarker>> markers) {
    rapidjson::Document::AllocatorType& allocator = jsonDocument.GetAllocator();

    rapidjson::Value jsonArray(rapidjson::kArrayType);
    for (auto marker : markers) {
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.SetObject();
        obj.AddMember("name", rapidjson::StringRef(marker->GetName()), allocator);
        obj.AddMember("body name", rapidjson::StringRef(marker->GetBody()->GetName()), allocator);
        obj.AddMember("frame position", true, allocator);
        obj.AddMember("frame orientation", true, allocator);
        jsonArray.PushBack(obj, allocator);
    }
    jsonDocument.AddMember("markers", jsonArray, allocator);
}

void ChPart::ExportLinSpringOutputChannels(rapidjson::Document& jsonDocument,
                                           std::vector<std::shared_ptr<ChLinkSpringCB>> springs) {
    rapidjson::Document::AllocatorType& allocator = jsonDocument.GetAllocator();

    rapidjson::Value jsonArray(rapidjson::kArrayType);
    for (auto spring : springs) {
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.SetObject();
        obj.AddMember("name", rapidjson::StringRef(spring->GetName()), allocator);
        obj.AddMember("length", true, allocator);
        obj.AddMember("force", true, allocator);
        jsonArray.PushBack(obj, allocator);
    }
    jsonDocument.AddMember("linear spring-dampers", jsonArray, allocator);
}

void ChPart::ExportRotSpringOutputChannels(rapidjson::Document& jsonDocument,
                                           std::vector<std::shared_ptr<ChLinkRotSpringCB>> springs) {
    rapidjson::Document::AllocatorType& allocator = jsonDocument.GetAllocator();

    rapidjson::Value jsonArray(rapidjson::kArrayType);
    for (auto spring : springs) {
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.SetObject();
        obj.AddMember("name", rapidjson::StringRef(spring->GetName()), allocator);
        obj.AddMember("angle", true, allocator);
        obj.AddMember("torque", true, allocator);
        jsonArray.PushBack(obj, allocator);
    }
    jsonDocument.AddMember("rotational spring-dampers", jsonArray, allocator);
}

}  // end namespace vehicle
}  // end namespace chrono
