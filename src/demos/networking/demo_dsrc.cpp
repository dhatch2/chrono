#include <iostream>
#include "ChNetworkHandler.h"
#include "ChronoMessages.pb.h"
#include "MessageConversions.h"
#include "ServerVehicle.h"
#include "ChDSRCDriver.h"
#include "DSRCNode.h"

#include "chrono/core/ChFileutils.h"
#include "chrono/core/ChStream.h"
#include "chrono/core/ChRealtimeStep.h"
#include "chrono/physics/ChSystem.h"
#include "chrono/utils/ChUtilsInputOutput.h"

#include "chrono_vehicle/ChConfigVehicle.h"
#include "chrono_vehicle/ChVehicleModelData.h"
#include "chrono_vehicle/terrain/RigidTerrain.h"
#include "chrono_vehicle/driver/ChIrrGuiDriver.h"
#include "chrono_vehicle/driver/ChDataDriver.h"
#include "chrono_vehicle/wheeled_vehicle/utils/ChWheeledVehicleIrrApp.h"

#include "chrono_models/vehicle/hmmwv/HMMWV.h"

using namespace chrono;
using namespace chrono::irrlicht;
using namespace chrono::vehicle;
using namespace chrono::vehicle::hmmwv;
using namespace irr;

// Initial vehicle location and orientation
ChVector<> initLoc(0, 0, 1.6);
ChQuaternion<> initRot(1, 0, 0, 0);
// ChQuaternion<> initRot(0.866025, 0, 0, 0.5);
// ChQuaternion<> initRot(0.7071068, 0, 0, 0.7071068);
// ChQuaternion<> initRot(0.25882, 0, 0, 0.965926);
// ChQuaternion<> initRot(0, 0, 0, 1);

enum DriverMode { DEFAULT, RECORD, PLAYBACK };
DriverMode driver_mode = DEFAULT;

// Visualization type for vehicle parts (PRIMITIVES, MESH, or NONE)
VisualizationType chassis_vis_type = VisualizationType::PRIMITIVES;
VisualizationType suspension_vis_type = VisualizationType::PRIMITIVES;
VisualizationType steering_vis_type = VisualizationType::PRIMITIVES;
VisualizationType wheel_vis_type = VisualizationType::NONE;

// Type of powertrain model (SHAFTS, SIMPLE)
PowertrainModelType powertrain_model = PowertrainModelType::SHAFTS;

// Drive type (FWD, RWD, or AWD)
DrivelineType drive_type = DrivelineType::AWD;

// Type of tire model (RIGID, RIGID_MESH, PACEJKA, LUGRE, FIALA)
TireModelType tire_model = TireModelType::RIGID;

// Rigid terrain
RigidTerrain::Type terrain_model = RigidTerrain::FLAT;
bool terrain_vis = true;
double terrainHeight = 0;      // terrain height (FLAT terrain only)
double terrainLength = 1000.0;  // size in X direction
double terrainWidth = 1000.0;   // size in Y direction

// Point on chassis tracked by the camera
ChVector<> trackPoint(0.0, 0.0, 1.75);

// Contact method
ChMaterialSurface::ContactMethod contact_method = ChMaterialSurface::SMC;
bool contact_vis = false;

// Simulation step sizes
double step_size = 1e-3;
double tire_step_size = step_size;

// Simulation end time
double t_end = 1000;

// Time interval between two render frames
double render_step_size = 1.0 / 50;  // FPS = 50

// Output directories
const std::string out_dir = "../HMMWV";
const std::string pov_dir = out_dir + "/POVRAY";

// Debug logging
bool debug_output = false;
double debug_step_size = 1.0 / 1;  // FPS = 1

// POV-Ray output
bool povray_output = false;

// =============================================================================

// TODO: Make a map-structure storing messages and their corresponding world object -- while preserving as much generality as possible.

int main(int argc, char **argv) {
    if (argc < 5 || argc > 5) {
        std::cout << "Usage: " << std::string(argv[0]) << " <ServerHostname> <port number> <DSRCHostname> <DSRC port number>" << std::endl;
        return 1;
    }

    //vehicle::SetDataPath("../../data/vehicle/");
    std::cout << GetDataPath() << std::endl;

    std::cout << "Connecting to network..." << std::endl;
    ChClientHandler handler(argv[1], argv[2]);
    handler.beginSend();
    handler.beginListen();

    std::cout << "Connected." << std::endl;

    ChClientHandler dsrcHandler(argv[3], argv[4]);
    dsrcHandler.beginSend();
    dsrcHandler.beginListen();

    ChronoMessages::DSRCMessage DSRCMess;
    DSRCMess.set_timestamp(0);
    DSRCMess.set_chtime(0);
    DSRCMess.set_idnumber(handler.connectionNumber());
    messageFromVector(DSRCMess.mutable_vehiclepos(), ChVector<>(0, 0, 0));
    ChronoMessages::PositionUpdate mess;
    messageFromVector(mess.mutable_position(), ChVector<>(0, 0, 0));
    mess.set_idnumber(handler.connectionNumber());
    //mess.CheckInitialized();
    DSRCMess.set_buffer(mess.SerializeAsString());
    //DSRCMess.CheckInitialized();
    //dsrcNode.send(DSRCMess);
    dsrcHandler.pushMessage(DSRCMess);


    /*DSRCNode dsrcNode(argv[3], argv[4]);
    dsrcNode.startReceive();
    dsrcNode.startSend();*/

    // Create map of vehicles received over the network
    std::map<std::pair<int, int>, std::shared_ptr<ServerVehicle>> otherVehicles;

    // --------------
    // Create systems
    // --------------
    // Create the HMMWV vehicle, set parameters, and initialize
    HMMWV_Full my_hmmwv;
    my_hmmwv.SetContactMethod(contact_method);
    my_hmmwv.SetChassisFixed(false);
    my_hmmwv.SetInitPosition(ChCoordsys<>(initLoc, initRot));
    my_hmmwv.SetPowertrainType(powertrain_model);
    my_hmmwv.SetDriveType(drive_type);
    my_hmmwv.SetTireType(tire_model);
    my_hmmwv.SetTireStepSize(tire_step_size);
    my_hmmwv.SetPacejkaParamfile("hmmwv/tire/HMMWV_pacejka.tir");
    my_hmmwv.Initialize();

    VisualizationType tire_vis_type =
        (tire_model == TireModelType::RIGID_MESH) ? VisualizationType::MESH : VisualizationType::PRIMITIVES;

    my_hmmwv.SetChassisVisualizationType(chassis_vis_type);
    my_hmmwv.SetSuspensionVisualizationType(suspension_vis_type);
    my_hmmwv.SetSteeringVisualizationType(steering_vis_type);
    my_hmmwv.SetWheelVisualizationType(wheel_vis_type);
    my_hmmwv.SetTireVisualizationType(tire_vis_type);

    // Create the terrain
    RigidTerrain terrain(my_hmmwv.GetSystem());
    terrain.SetContactFrictionCoefficient(0.9f);
    terrain.SetContactRestitutionCoefficient(0.01f);
    terrain.SetContactMaterialProperties(2e7f, 0.3f);
    terrain.SetColor(ChColor(0.8f, 0.8f, 0.5f));
    terrain.EnableVisualization(terrain_vis);
    std::cout << GetDataPath() << std::endl;
    std::cout << GetDataFile("terrain/textures/tile4.jpg") << std::endl;
    std::cout << GetDataFile("terrain/textures/grass.jpg") << std::endl;

    switch (terrain_model) {
        case RigidTerrain::FLAT:
            terrain.SetTexture(vehicle::GetDataFile("terrain/textures/tile4.jpg"), 200, 200);
            terrain.Initialize(terrainHeight, terrainLength, terrainWidth);
            break;
        case RigidTerrain::HEIGHT_MAP:
            terrain.SetTexture(vehicle::GetDataFile("terrain/textures/grass.jpg"), 16, 16);
            terrain.Initialize(vehicle::GetDataFile("terrain/height_maps/test64.bmp"), "test64", 128, 128, 0, 4);
            break;
        case RigidTerrain::MESH:
            terrain.SetTexture(vehicle::GetDataFile("terrain/textures/grass.jpg"), 100, 100);
            terrain.Initialize(vehicle::GetDataFile("terrain/meshes/test.obj"), "test_mesh");
            break;
    }

        // Create the vehicle Irrlicht interface
    ChWheeledVehicleIrrApp app(&my_hmmwv.GetVehicle(), &my_hmmwv.GetPowertrain(),
                                 L"HMMWV Demo");
    app.SetSkyBox();
    auto lightNode = app.AddLightWithShadow(irr::core::vector3df(0.f, 0.f, 100.f),
                             irr::core::vector3df(0.f, 0.f, 0.f), 1000.0, 50.0,
                             150.0, 90.0, 512,
                             irr::video::SColorf(1.f, 1.f, 1.f, 1.f), true, true);
    lightNode->getLightData().AmbientColor.set(1.f, 0.07f, 0.07f, 0.07f);
    app.SetChaseCamera(trackPoint, 6.0, 0.5);
    app.SetTimestep(step_size);
    app.AssetBindAll();
    scene::IMeshSceneNode* node = app.GetSceneManager()->addMeshSceneNode(
          app.GetSceneManager()->getMesh("../data/madison_flat_mod.obj"), 0, -1,
          core::vector3df(150, 250, 0.01), core::vector3df(0, 0, 0),
          core::vector3df(1.0, 1.0, 1.0));
    app.AssetUpdateAll();

    // -----------------
    // Initialize output
    // -----------------

    if (ChFileutils::MakeDirectory(out_dir.c_str()) < 0) {
        std::cout << "Error creating directory " << out_dir << std::endl;
        return 1;
    }
    if (povray_output) {
        if (ChFileutils::MakeDirectory(pov_dir.c_str()) < 0) {
            std::cout << "Error creating directory " << pov_dir << std::endl;
            return 1;
        }
        terrain.ExportMeshPovray(out_dir);
    }

    std::string driver_file = out_dir + "/driver_inputs.txt";
    utils::CSV_writer driver_csv(" ");

    // ------------------------
    // Create the driver system
    // ------------------------

    ChVector<> target(0, 0, 0);

    // Create the interactive driver system
    ChDSRCDriver driver(my_hmmwv.GetVehicle(), target, 80, 3, 10, 3);

    // Set the time response for steering and throttle keyboard inputs.
    double steering_time = 1.0;  // time to go from 0 to +1 (or from 0 to -1)
    double throttle_time = 1.0;  // time to go from 0 to +1
    double braking_time = 0.3;   // time to go from 0 to +1
    //driver.SetSteeringDelta(render_step_size / steering_time);
    //driver.SetThrottleDelta(render_step_size / throttle_time);
    //driver.SetBrakingDelta(render_step_size / braking_time);

    // If in playback mode, attach the data file to the driver system and
    // force it to playback the driver inputs.
    //if (driver_mode == PLAYBACK) {
    //    driver.SetInputDataFile(driver_file);
    //    driver.SetInputMode(ChIrrGuiDriver::DATAFILE);
    //}

    driver.Initialize();

    // ---------------
    // Simulation loop
    // ---------------

    if (debug_output) {
        GetLog() << "\n\n============ System Configuration ============\n";
        my_hmmwv.LogHardpointLocations();
    }

    // Number of simulation steps between miscellaneous events
    int render_steps = (int)std::ceil(render_step_size / step_size);
    int debug_steps = (int)std::ceil(debug_step_size / step_size);
    int send_steps = 1;

    // Initialize simulation frame counter and simulation time
    ChRealtimeStepTimer realtime_timer;
    int step_number = 0;
    int render_frame = 0;
    double time = 0;

    if (contact_vis) {
        app.SetSymbolscale(1e-4);
        app.SetContactsDrawMode(ChIrrTools::eCh_ContactsDrawMode::CONTACT_FORCES);
    }

    while (app.GetDevice()->run()) {
        time = my_hmmwv.GetSystem()->GetChTime();

        // End simulation
        if (time >= t_end)
            break;

        // Render scene and output POV-Ray data
        if (step_number % render_steps == 0) {
            app.BeginScene(true, true, irr::video::SColor(255, 140, 161, 192));
            app.DrawAll();
            app.EndScene();

            if (povray_output) {
                char filename[100];
                sprintf(filename, "%s/data_%03d.dat", pov_dir.c_str(), render_frame + 1);
                utils::WriteShapesPovray(my_hmmwv.GetSystem(), filename);
            }

            render_frame++;
        }

        // Debug logging
        if (debug_output && step_number % debug_steps == 0) {
            GetLog() << "\n\n============ System Information ============\n";
            GetLog() << "Time = " << time << "\n\n";
            my_hmmwv.DebugLog(OUT_SPRINGS | OUT_SHOCKS | OUT_CONSTRAINTS);
        }

        // Collect output data from modules (for inter-module communication)
        double throttle_input = driver.GetThrottle();
        double steering_input = driver.GetSteering();
        double braking_input = driver.GetBraking();

        // Driver output
        if (driver_mode == RECORD) {
            driver_csv << time << steering_input << throttle_input << braking_input << std::endl;
        }

        // Update modules (process inputs from other modules)
        driver.Synchronize(time);
        terrain.Synchronize(time);
        my_hmmwv.Synchronize(time, steering_input, braking_input, throttle_input, terrain);
        app.Synchronize("AUTONOMOUS", steering_input, throttle_input, braking_input);

        if (step_number % send_steps == 0) {
            auto message = generateVehicleMessageFromWheeledVehicle(&my_hmmwv.GetVehicle(), handler.connectionNumber(), 0);
            handler.pushMessage(message);

            while (handler.waitingMessages() > 0) {
                auto newMessage = std::static_pointer_cast<ChronoMessages::VehicleMessage>(handler.popSimMessage()); // TODO: Make this crap more general
                if (newMessage->connectionnumber() != handler.connectionNumber()) {
                    // TODO: Do thing with maps or something
                    auto idPair = std::make_pair(newMessage->connectionnumber(), newMessage->idnumber());
                    if (otherVehicles.find(idPair) == otherVehicles.end()) {
                        auto newVehicle = std::make_shared<ServerVehicle>(my_hmmwv.GetVehicle().GetSystem());
                        otherVehicles.insert(std::make_pair(idPair, newVehicle));
                        app.AssetBindAll();
                        app.AssetUpdateAll();
                        newVehicle->update(*newMessage);
                        std::cout << "New vehicle updated." << std::endl;
                    } else otherVehicles[idPair]->update(*newMessage);
                    /*ChronoMessages::PositionUpdate posUp;
                    posUp.set_idnumber(newMessage->connectionnumber());
                    posUp.mutable_position()->set_x(newMessage->chassiscom().x());
                    posUp.mutable_position()->set_y(newMessage->chassiscom().y());
                    posUp.mutable_position()->set_z(newMessage->chassiscom().z());
                    std::string mess;
                    posUp.SerializeToString(&mess);
                    driver.Update(mess);*/
                }
            }

            while (dsrcHandler.waitingDSRCMessages() > 0) {
                driver.Update(dsrcHandler.popDSRCMessage()->buffer());
            }

            /*for (auto it = otherVehicles.begin(); it != otherVehicles.end(); it++) {
                if (otherVehicles.find(it->first) == otherVehicles.end()) {
                    it = otherVehicles.erase(it);
                } else ++it;
            }*/
        }

        // Advance simulation for one timestep for all modules
        app.SetTimestep(step_size); ////////////////////////////////////////// Some experimental stuff.
        double step = realtime_timer.SuggestSimulationStep(step_size);
        driver.Advance(step);
        terrain.Advance(step);
        my_hmmwv.Advance(step);
        app.Advance(step);
        render_steps = (int)std::ceil(render_step_size / step_size);

        // Increment frame number
        step_number++;
    }

    return 0;
}