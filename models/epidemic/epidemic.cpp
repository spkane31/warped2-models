#include "epidemic.hpp"
#include "WattsStrogatzModel.hpp"
#include "tclap/ValueArg.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using namespace boost::property_tree;

WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(EpidemicEvent)

std::vector<std::shared_ptr<warped::Event> > Location::createInitialEvents() {

    std::vector<std::shared_ptr<warped::Event> > events;
    events.emplace_back(new EpidemicEvent {this->location_name_, 
                    this->location_state_refresh_interval_, nullptr, DISEASE_UPDATE_TRIGGER});
    events.emplace_back(new EpidemicEvent {this->location_name_, 
                    this->location_diffusion_trigger_interval_, nullptr, DIFFUSION_TRIGGER});
    return events;
}

std::vector<std::shared_ptr<warped::Event> > Location::receiveEvent(const warped::Event& event) {

    std::vector<std::shared_ptr<warped::Event> > events;
    auto epidemic_event = static_cast<const EpidemicEvent&>(event);
    auto timestamp = epidemic_event.loc_arrival_timestamp_;

    switch (epidemic_event.event_type_) {

        case event_type_t::DISEASE_UPDATE_TRIGGER: {
            disease_model_->reaction(state_->current_population_, timestamp);
            events.emplace_back(new EpidemicEvent {location_name_, 
                                timestamp + location_state_refresh_interval_, 
                                nullptr, DISEASE_UPDATE_TRIGGER});
        } break;

        case event_type_t::DIFFUSION_TRIGGER: {
            std::string selected_location = diffusion_network_->pickLocation();
            if (selected_location != "") {
                auto travel_time = diffusion_network_->travelTimeToLocation(selected_location);
                unsigned int person_count = state_->current_population_->size();
                if (person_count) {
                    unsigned int person_id = diffusion_network_->pickPerson(person_count);
                    auto map_iter = state_->current_population_->begin();
                    unsigned int temp_cnt = 0;
                    while (temp_cnt < person_id) {
                        map_iter++;
                        temp_cnt++;
                    }
                    std::shared_ptr<Person> person = map_iter->second;
                    events.emplace_back(new EpidemicEvent {selected_location, 
                                            timestamp + travel_time, person, DIFFUSION});
                    state_->current_population_->erase(map_iter);
                }
            }
            events.emplace_back(new EpidemicEvent {location_name_, 
                                timestamp + location_diffusion_trigger_interval_, 
                                nullptr, DIFFUSION_TRIGGER});
        } break;

        case event_type_t::DIFFUSION: {
            std::shared_ptr<Person> person = std::make_shared<Person>(
                        epidemic_event.pid_, epidemic_event.susceptibility_, 
                        epidemic_event.vaccination_status_, epidemic_event.infection_state_,
                        timestamp, epidemic_event.prev_state_change_timestamp_);
            state_->current_population_->insert(state_->current_population_->begin(), 
                std::pair <unsigned int, std::shared_ptr<Person>> (epidemic_event.pid_, person));
        } break;

        default: {}
    }
    return events;
}

int main(int argc, const char** argv) {

    unsigned int num_regions = 100;
    unsigned int num_locations_per_region = 100;
    unsigned int num_persons_per_location = 100000;

    TCLAP::ValueArg<unsigned int> num_regions_arg("r", "num-regions", "Number of regions",
                                                            false, num_regions, "unsigned int");
    TCLAP::ValueArg<unsigned int> num_locations_per_region_arg("l", "num-locations-per-region", 
                "Number of locations per region", false, num_locations_per_region, "unsigned int");
    TCLAP::ValueArg<unsigned int> num_persons_per_location_arg("p", "num-persons-per-location", 
                "Number of persons per location", false, num_persons_per_location, "unsigned int");

    std::vector<TCLAP::Arg*> args = {&num_regions_arg, &num_locations_per_region_arg, 
                                                                &num_persons_per_location_arg};

    warped::Simulation epidemic_sim {"Epidemic Simulation", argc, argv, args};

    num_regions                 = num_regions_arg.getValue();
    num_locations_per_region    = num_locations_per_region_arg.getValue();
    num_persons_per_location    = num_persons_per_location_arg.getValue();

    // Diffusion model
    unsigned int diffusion_seed = 101;
    unsigned int k = 8;
    float beta = 0.1;

    // Disease
    float transmissibility = 0.12;
    unsigned int latent_dwell_time = 200;
    float latent_infectivity = 0;
    unsigned int incubating_dwell_time = 400;
    float incubating_infectivity = 0.3;
    unsigned int infectious_dwell_time = 400;
    float infectious_infectivity = 1.0;
    unsigned int asympt_dwell_time = 200;
    float asympt_infectivity = 0.5;
    float prob_ulu = 0.2;
    float prob_ulv = 0.9;
    float prob_urv = 0.5;
    float prob_uiv = 0.1;
    float prob_uiu = 0.3;
    unsigned int loc_state_refresh_interval = 50;
    unsigned int disease_seed = 90;

    unsigned long pid = 0;
    std::map<std::string, unsigned int> travel_map;
    std::vector<Location> objects;

    for (unsigned int region_id = 0; region_id < num_regions; region_id++) {
        std::string region_name = std::string("region_") + std::to_string(region_id);

        for (unsigned int location_id = 0; location_id < num_locations_per_region; location_id++) {
            std::string location_name = std::string("location_") + std::to_string(location_id);
            std::string location = region_name + std::string("-") + location_name;
            std::vector<std::shared_ptr<Person>> population;
            unsigned int travel_time_to_hub = 200;
            travel_map.insert(std::pair<std::string, unsigned int>(location, travel_time_to_hub));
            unsigned int loc_diffusion_trig_interval = 50;

            for (unsigned int person_id = 0; person_id < num_persons_per_location; person_id++) {
                pid++;
                std::string person_pid = std::string("person_") +std::to_string(pid);
                double susceptibility = 0.0;
                bool vaccination_status = false;
                std::string infection_state = "uninfected";

                infection_state_t state;
                if (infection_state == "uninfected") {
                    state = UNINFECTED;
                } else if (infection_state == "latent") {
                    state = LATENT;
                } else if (infection_state == "incubating") {
                    state = INCUBATING;
                } else if (infection_state == "infectious") {
                    state = INFECTIOUS;
                } else if (infection_state == "asympt") {
                    state = ASYMPT;
                } else {
                    state = RECOVERED;
                }

                auto person = std::make_shared<Person>( 
                                    pid, 
                                    susceptibility, 
                                    vaccination_status, 
                                    state, 
                                    0, 
                                    0
                                );
                population.push_back(person);
            }
            objects.emplace_back(   location, 
                                    transmissibility, 
                                    latent_dwell_time, 
                                    incubating_dwell_time, 
                                    infectious_dwell_time, 
                                    asympt_dwell_time, 
                                    latent_infectivity, 
                                    incubating_infectivity, 
                                    infectious_infectivity, 
                                    asympt_infectivity, 
                                    prob_ulu, 
                                    prob_ulv, 
                                    prob_urv, 
                                    prob_uiv, 
                                    prob_uiu, 
                                    loc_state_refresh_interval, 
                                    loc_diffusion_trig_interval, 
                                    population, 
                                    travel_time_to_hub, 
                                    disease_seed, 
                                    diffusion_seed
                                );

        }
    }

    // Create the Watts-Strogatz model
    auto ws = std::make_shared<WattsStrogatzModel>(k, beta, diffusion_seed);
    std::vector<std::string> nodes;
    for (auto& o : objects) {
        nodes.push_back(o.getLocationName());
    }
    ws->populateNodes(nodes);
    ws->mapNodes();

    // Create the travel map
    for (auto& o : objects) {
        std::vector<std::string> connections = ws->fetchNodeLinks(o.getLocationName());
        std::map<std::string, unsigned int> temp_travel_map;
        for (auto& link : connections) {
            auto travel_map_iter = travel_map.find(link);
            temp_travel_map.insert(std::pair<std::string, unsigned int>
                                (travel_map_iter->first, travel_map_iter->second));
        }
        o.populateTravelDistances(temp_travel_map);
    }

    std::vector<warped::SimulationObject*> object_pointers;
    for (auto& o : objects) {
        object_pointers.push_back(&o);
    }
    epidemic_sim.simulate(object_pointers);

    return 0;
}
