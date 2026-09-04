#include <iterator>

#define main udonshield_frontier_297_frozen_main
#include "bidirectional_rcsp_probe.cpp"
#undef main

namespace boundary {

constexpr const char* kBoundaryExperiment = "ATTR-BIDIRECTIONAL-RCSP-BOUNDARY-298";

struct Row {
    std::string family;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t agentCount = 0;
    std::int32_t spotCount = 0;
    std::int32_t players = 0;
    std::int32_t daySteps = 0;
    std::string fuelProfile;
    std::string startMode;
    std::string targetMode;
    std::uint64_t firstSeed = 0;
    std::int32_t count = 0;
};

struct BoundaryOptions {
    std::string manifest = "research/holdouts/ATTR-BIDIRECTIONAL-RCSP-BOUNDARY-298.csv";
    std::string split = "development";
    std::optional<std::uint64_t> onlySeed;
    bool details = false;
};

[[nodiscard]] BoundaryOptions parse_boundary_options(std::int32_t argc, char** argv) {
    BoundaryOptions options;
    for (std::int32_t index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--manifest" && index + 1 < argc) {
            options.manifest = argv[++index];
        } else if (argument == "--split" && index + 1 < argc) {
            options.split = argv[++index];
        } else if (argument == "--only-seed" && index + 1 < argc) {
            options.onlySeed = std::stoull(argv[++index]);
        } else if (argument == "--details") {
            options.details = true;
        } else {
            throw std::invalid_argument("unknown or incomplete option: " + argument);
        }
    }
    if (options.split != "development" && options.split != "verification") {
        throw std::invalid_argument("split must be development or verification");
    }
    return options;
}

[[nodiscard]] std::vector<Row> load_boundary_manifest(
    const std::string& path,
    const std::string& requestedSplit) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open manifest: " + path);
    }
    std::string line;
    constexpr const char* header =
        "experiment_id,split,family,width,height,agent_count,spot_count,players,day_steps,fuel_profile,start_mode,target_mode,first_seed,count,scope";
    if (!std::getline(input, line) || line != header) {
        throw std::runtime_error("unexpected boundary manifest schema");
    }
    std::vector<Row> rows;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_csv(line);
        if (fields.size() != 15U || fields.at(0) != kBoundaryExperiment ||
            fields.at(14) != "exact-required-terminal-frontier") {
            throw std::runtime_error("invalid boundary manifest row: " + line);
        }
        if (fields.at(1) != requestedSplit) {
            continue;
        }
        Row row;
        row.family = fields.at(2);
        row.width = std::stoi(fields.at(3));
        row.height = std::stoi(fields.at(4));
        row.agentCount = std::stoi(fields.at(5));
        row.spotCount = std::stoi(fields.at(6));
        row.players = std::stoi(fields.at(7));
        row.daySteps = std::stoi(fields.at(8));
        row.fuelProfile = fields.at(9);
        row.startMode = fields.at(10);
        row.targetMode = fields.at(11);
        row.firstSeed = std::stoull(fields.at(12));
        row.count = std::stoi(fields.at(13));
        if ((row.startMode != "spot" && row.startMode != "nonspot") ||
            (row.targetMode != "spot" && row.targetMode != "nonspot") ||
            row.spotCount > 12 || row.count <= 0) {
            throw std::runtime_error("boundary row violates registered scope: " + line);
        }
        rows.push_back(std::move(row));
    }
    if (rows.empty()) {
        throw std::runtime_error("manifest contains no rows for split " + requestedSplit);
    }
    return rows;
}

struct BoundaryFixture {
    Fixture base;
    udon::CellId target = udon::kInvalidCell;
};

[[nodiscard]] BoundaryFixture make_boundary_fixture(const Row& row, std::uint64_t seed) {
    ManifestRow baseRow;
    baseRow.family = row.family;
    baseRow.width = row.width;
    baseRow.height = row.height;
    baseRow.agentCount = row.agentCount;
    baseRow.spotCount = row.spotCount;
    baseRow.players = row.players;
    baseRow.daySteps = row.daySteps;
    baseRow.fuelProfile = row.fuelProfile;
    baseRow.firstSeed = seed;
    baseRow.count = 1;
    BoundaryFixture fixture{make_fixture(baseRow, seed), udon::kInvalidCell};
    if (row.startMode == "spot") {
        fixture.base.start = fixture.base.config.spots.front().position;
        fixture.base.state.agents.front().position = fixture.base.start;
    }
    if (row.targetMode == "spot") {
        fixture.target = fixture.base.config.spots.back().position;
    } else {
        const std::int32_t cells = fixture.base.config.map.cell_count();
        std::uint64_t cursor = seed ^ 0xa0761d6478bd642fULL;
        for (std::int32_t attempt = 0; attempt < cells * 2; ++attempt) {
            cursor = mix64(cursor);
            const udon::CellId candidate = static_cast<udon::CellId>(
                cursor % static_cast<std::uint64_t>(cells));
            if (fixture.base.config.map.terrain.at(static_cast<std::size_t>(candidate)) !=
                    udon::Terrain::Pond &&
                fixture.base.config.spotAtCell.at(static_cast<std::size_t>(candidate)) ==
                    udon::kInvalidSpot &&
                candidate != fixture.base.start) {
                fixture.target = candidate;
                break;
            }
        }
    }
    if (fixture.target == udon::kInvalidCell) {
        throw std::runtime_error("failed to choose required terminal");
    }
    return fixture;
}

[[nodiscard]] SearchResult boundary_forward(
    const Fixture& fixture,
    udon::CellId target,
    std::int32_t stepCap,
    bool reverseDirections,
    bool useTargetBounds) {
    const udon::MatchConfig& config = fixture.config;
    SearchResult result;
    result.atState.resize(
        (std::size_t{1} << config.spots.size()) *
        static_cast<std::size_t>(config.map.cell_count()));
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> queue;
    static_cast<void>(insert_label(
        config,
        result,
        Label{fixture.start, 0U},
        queue));
    const std::int32_t root = 0;
    const std::uint32_t startBit = mask_at(config, fixture.start);
    if (startBit != 0U && stepCap >= 1) {
        static_cast<void>(insert_label(
            config,
            result,
            Label{fixture.start, startBit, 1, 0, root, udon::kDirectionCount, true},
            queue));
    }
    TargetBounds bounds;
    if (useTargetBounds) {
        bounds = target_bounds(fixture, target);
        result.work += bounds.work;
    }
    while (!queue.empty()) {
        const QueueItem item = queue.top();
        queue.pop();
        const Label label = result.labels.at(static_cast<std::size_t>(item.label));
        if (!label.active) {
            continue;
        }
        ++result.work.settled;
        const udon::MoveCost cost = config.move_cost(
            label.cell,
            fixture.state.roadStatuses.at(static_cast<std::size_t>(label.cell)));
        for (std::int32_t order = 0; order < udon::kDirectionCount; ++order) {
            ++result.work.arcScans;
            const std::int32_t direction = reverseDirections
                ? udon::kDirectionCount - 1 - order
                : order;
            const udon::CellId next = config.map.neighbors.at(static_cast<std::size_t>(label.cell))
                .at(static_cast<std::size_t>(direction));
            if (next == udon::kInvalidCell ||
                config.map.terrain.at(static_cast<std::size_t>(next)) == udon::Terrain::Pond) {
                continue;
            }
            const std::int32_t nextSteps = label.steps + cost.steps;
            const std::int32_t nextFuel = label.fuel + cost.patrolFuel;
            if (nextSteps > stepCap || nextFuel > config.fuelLimit) {
                continue;
            }
            if (useTargetBounds &&
                (bounds.steps.at(static_cast<std::size_t>(next)) > stepCap - nextSteps ||
                 bounds.fuel.at(static_cast<std::size_t>(next)) > config.fuelLimit - nextFuel)) {
                continue;
            }
            static_cast<void>(insert_label(config, result, Label{
                next,
                label.mask | mask_at(config, next),
                nextSteps,
                nextFuel,
                item.label,
                direction,
                true,
            }, queue));
        }
    }
    return result;
}

[[nodiscard]] SearchResult boundary_backward(
    const Fixture& fixture,
    udon::CellId target,
    std::int32_t stepCap,
    bool reverseDirections,
    std::int32_t& reverseCostFailures) {
    const udon::MatchConfig& config = fixture.config;
    SearchResult result;
    result.atState.resize(
        (std::size_t{1} << config.spots.size()) *
        static_cast<std::size_t>(config.map.cell_count()));
    std::vector<std::vector<std::pair<udon::CellId, std::int32_t>>> incoming(
        static_cast<std::size_t>(config.map.cell_count()));
    for (udon::CellId source = 0; source < config.map.cell_count(); ++source) {
        if (config.map.terrain.at(static_cast<std::size_t>(source)) == udon::Terrain::Pond) {
            continue;
        }
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            ++result.work.arcScans;
            const udon::CellId destination = config.map.neighbors.at(static_cast<std::size_t>(source))
                .at(static_cast<std::size_t>(direction));
            if (destination != udon::kInvalidCell &&
                config.map.terrain.at(static_cast<std::size_t>(destination)) != udon::Terrain::Pond) {
                incoming.at(static_cast<std::size_t>(destination)).emplace_back(source, direction);
            }
        }
    }
    if (reverseDirections) {
        for (auto& bucket : incoming) {
            std::reverse(bucket.begin(), bucket.end());
        }
    }
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> queue;
    static_cast<void>(insert_label(config, result, Label{target, 0U}, queue));
    while (!queue.empty()) {
        const QueueItem item = queue.top();
        queue.pop();
        const Label label = result.labels.at(static_cast<std::size_t>(item.label));
        if (!label.active) {
            continue;
        }
        ++result.work.settled;
        for (const auto& [predecessor, direction] :
             incoming.at(static_cast<std::size_t>(label.cell))) {
            ++result.work.arcScans;
            const udon::MoveCost cost = config.move_cost(
                predecessor,
                fixture.state.roadStatuses.at(static_cast<std::size_t>(predecessor)));
            if (config.map.neighbors.at(static_cast<std::size_t>(predecessor))
                    .at(static_cast<std::size_t>(direction)) != label.cell ||
                cost.steps <= 0 || cost.patrolFuel < 0) {
                ++reverseCostFailures;
            }
            const std::int32_t nextSteps = label.steps + cost.steps;
            const std::int32_t nextFuel = label.fuel + cost.patrolFuel;
            if (nextSteps > stepCap || nextFuel > config.fuelLimit) {
                continue;
            }
            static_cast<void>(insert_label(config, result, Label{
                predecessor,
                label.mask | mask_at(config, label.cell),
                nextSteps,
                nextFuel,
                item.label,
                direction,
                true,
            }, queue));
        }
    }
    return result;
}

[[nodiscard]] std::vector<FrontierEntry> boundary_frontier(
    const SearchResult& search,
    udon::CellId target) {
    std::vector<FrontierEntry> candidates;
    for (std::int32_t index = 0; index < static_cast<std::int32_t>(search.labels.size()); ++index) {
        const Label& label = search.labels.at(static_cast<std::size_t>(index));
        if (label.active && label.cell == target && std::popcount(label.mask) > 0) {
            candidates.push_back(FrontierEntry{
                label.mask,
                label.steps,
                label.fuel,
                reconstruct_forward(search, index),
            });
        }
    }
    return pareto_frontier(std::move(candidates));
}

[[nodiscard]] TargetResult boundary_bidirectional(
    const Fixture& fixture,
    udon::CellId target,
    bool reverseDirections) {
    TargetResult result;
    const std::int32_t budget = fixture.config.steps_for_day(fixture.state.dayNumber);
    const std::int32_t forwardCap = budget / 2 + 4;
    const std::int32_t backwardCap = budget - budget / 2;
    SearchResult forward = boundary_forward(
        fixture,
        target,
        forwardCap,
        reverseDirections,
        false);
    SearchResult backward = boundary_backward(
        fixture,
        target,
        backwardCap,
        reverseDirections,
        result.reverseCostFailures);
    result.work += forward.work;
    result.work += backward.work;
    std::vector<std::vector<std::int32_t>> reverseAtCell(
        static_cast<std::size_t>(fixture.config.map.cell_count()));
    for (std::int32_t index = 0; index < static_cast<std::int32_t>(backward.labels.size()); ++index) {
        const Label& label = backward.labels.at(static_cast<std::size_t>(index));
        if (label.active) {
            reverseAtCell.at(static_cast<std::size_t>(label.cell)).push_back(index);
        }
    }
    std::vector<FrontierEntry> candidates;
    for (std::int32_t forwardIndex = 0;
         forwardIndex < static_cast<std::int32_t>(forward.labels.size());
         ++forwardIndex) {
        const Label& prefix = forward.labels.at(static_cast<std::size_t>(forwardIndex));
        if (!prefix.active) {
            continue;
        }
        for (const std::int32_t backwardIndex :
             reverseAtCell.at(static_cast<std::size_t>(prefix.cell))) {
            ++result.work.joinPairs;
            const Label& suffix = backward.labels.at(static_cast<std::size_t>(backwardIndex));
            const std::int32_t steps = prefix.steps + suffix.steps;
            const std::int32_t fuel = prefix.fuel + suffix.fuel;
            if (steps > budget || fuel > fixture.config.fuelLimit) {
                continue;
            }
            std::vector<std::int32_t> directions = reconstruct_forward(forward, forwardIndex);
            std::vector<std::int32_t> tail = reconstruct_backward(backward, backwardIndex);
            directions.insert(directions.end(), tail.begin(), tail.end());
            if (std::popcount(prefix.mask | suffix.mask) > 0) {
                candidates.push_back(FrontierEntry{
                    prefix.mask | suffix.mask,
                    steps,
                    fuel,
                    std::move(directions),
                });
            }
        }
    }
    result.frontier = pareto_frontier(std::move(candidates));
    return result;
}

[[nodiscard]] bool boundary_validate(
    const Fixture& fixture,
    const FrontierEntry& entry,
    udon::CellId target,
    std::string& mismatch) {
    udon::DayPlan plan;
    plan.actions.resize(static_cast<std::size_t>(fixture.config.agent_count()));
    for (const std::int32_t direction : entry.directions) {
        plan.actions.front().push_back(
            direction == udon::kDirectionCount
                ? udon::PlanAction::wait(1)
                : udon::PlanAction::move(direction));
    }
    const std::int32_t remaining =
        fixture.config.steps_for_day(fixture.state.dayNumber) - entry.steps;
    if (remaining > 0) {
        plan.actions.front().push_back(udon::PlanAction::wait(remaining));
    }
    for (std::int32_t agent = 1; agent < fixture.config.agent_count(); ++agent) {
        plan.actions.at(static_cast<std::size_t>(agent)).push_back(
            udon::PlanAction::wait(fixture.config.steps_for_day(fixture.state.dayNumber)));
    }
    const udon::ExactStepSimulator simulator(fixture.config);
    const udon::IndependentDayValidator validator(fixture.config);
    const udon::SimulationResult simulation = simulator.simulate(fixture.state, plan, false);
    const udon::SimulationResult independent = validator.validate(fixture.state, plan, false);
    if (!simulation.valid || !independent.valid ||
        !validator.agrees_with(simulation, independent, mismatch)) {
        return false;
    }
    if (simulation.finalAgents.front().position != target ||
        simulation.finalAgents.front().fuel != fixture.config.fuelLimit - entry.fuel) {
        mismatch = "terminal tuple mismatch";
        return false;
    }
    std::uint32_t observed = 0U;
    for (const udon::ClaimEvent& claim : simulation.claims) {
        if (claim.agent == 0 && claim.served) {
            observed |= std::uint32_t{1} << static_cast<std::uint32_t>(claim.spot);
        }
    }
    if (observed != entry.mask) {
        mismatch = "destination-claim mask mismatch";
        return false;
    }
    return true;
}

[[nodiscard]] std::uint64_t tuple_hash(const std::vector<FrontierEntry>& frontier) {
    std::uint64_t hash = kFnvOffset;
    hash_frontier(hash, frontier);
    return hash;
}

} // namespace boundary

int main(int argc, char** argv) {
    try {
        const boundary::BoundaryOptions options = boundary::parse_boundary_options(argc, argv);
        const std::vector<boundary::Row> rows =
            boundary::load_boundary_manifest(options.manifest, options.split);
        std::int32_t cases = 0;
        std::int32_t frontierMismatch = 0;
        std::int32_t rankMismatch = 0;
        std::int32_t deterministicFailure = 0;
        std::int32_t invalid = 0;
        std::int32_t reverseCostFailure = 0;
        std::set<std::string> reducedFamilies;
        std::vector<double> ratios;
        std::uint64_t resultHash = kFnvOffset;
        for (const boundary::Row& row : rows) {
            for (std::int32_t offset = 0; offset < row.count; ++offset) {
                const std::uint64_t seed = row.firstSeed + static_cast<std::uint64_t>(offset);
                if (options.onlySeed.has_value() && seed != *options.onlySeed) {
                    continue;
                }
                const boundary::BoundaryFixture boundaryFixture =
                    boundary::make_boundary_fixture(row, seed);
                const Fixture& fixture = boundaryFixture.base;
                SearchResult forward = boundary::boundary_forward(
                    fixture,
                    boundaryFixture.target,
                    fixture.config.steps_for_day(fixture.state.dayNumber),
                    false,
                    true);
                SearchResult forwardReversed = boundary::boundary_forward(
                    fixture,
                    boundaryFixture.target,
                    fixture.config.steps_for_day(fixture.state.dayNumber),
                    true,
                    true);
                const std::vector<FrontierEntry> reference =
                    boundary::boundary_frontier(forward, boundaryFixture.target);
                const std::vector<FrontierEntry> referenceReversed =
                    boundary::boundary_frontier(forwardReversed, boundaryFixture.target);
                TargetResult bidirectional = boundary::boundary_bidirectional(
                    fixture,
                    boundaryFixture.target,
                    false);
                TargetResult bidirectionalReversed = boundary::boundary_bidirectional(
                    fixture,
                    boundaryFixture.target,
                    true);
                std::int32_t caseInvalid = 0;
                for (const FrontierEntry& entry : reference) {
                    std::string mismatch;
                    if (!boundary::boundary_validate(
                            fixture,
                            entry,
                            boundaryFixture.target,
                            mismatch)) {
                        ++caseInvalid;
                    }
                }
                for (const FrontierEntry& entry : bidirectional.frontier) {
                    std::string mismatch;
                    if (!boundary::boundary_validate(
                            fixture,
                            entry,
                            boundaryFixture.target,
                            mismatch)) {
                        ++caseInvalid;
                    }
                }
                const bool sameFrontier = keys(reference) == keys(bidirectional.frontier);
                const bool sameRank = best_rank(reference) == best_rank(bidirectional.frontier);
                const bool deterministic =
                    keys(reference) == keys(referenceReversed) &&
                    keys(bidirectional.frontier) == keys(bidirectionalReversed.frontier);
                const double ratio = forward.work.total() == 0U
                    ? 1.0
                    : static_cast<double>(bidirectional.work.total()) /
                        static_cast<double>(forward.work.total());
                if (ratio < 1.0) {
                    reducedFamilies.insert(row.family);
                }
                ratios.push_back(ratio);
                ++cases;
                frontierMismatch += sameFrontier ? 0 : 1;
                rankMismatch += sameRank ? 0 : 1;
                deterministicFailure += deterministic ? 0 : 1;
                invalid += caseInvalid;
                reverseCostFailure += bidirectional.reverseCostFailures +
                    bidirectionalReversed.reverseCostFailures;
                hash_value(resultHash, seed);
                hash_value(resultHash, boundary::tuple_hash(reference));
                hash_value(resultHash, boundary::tuple_hash(bidirectional.frontier));
                hash_value(resultHash, forward.work.total());
                hash_value(resultHash, bidirectional.work.total());
                std::cout << "case"
                          << " split=" << options.split
                          << " family=" << row.family
                          << " seed=" << seed
                          << " start_mode=" << row.startMode
                          << " target_mode=" << row.targetMode
                          << " frontier=" << reference.size()
                          << " forward_work=" << forward.work.total()
                          << " bidirectional_work=" << bidirectional.work.total()
                          << " work_ratio=" << std::fixed << std::setprecision(6) << ratio
                          << " frontier_mismatch=" << (sameFrontier ? 0 : 1)
                          << " best_rank_mismatch=" << (sameRank ? 0 : 1)
                          << " deterministic_failure=" << (deterministic ? 0 : 1)
                          << " invalid=" << caseInvalid
                          << " reverse_cost_failure=" <<
                              (bidirectional.reverseCostFailures +
                               bidirectionalReversed.reverseCostFailures)
                          << '\n';
                if (options.details) {
                    std::cout << "detail seed=" << seed
                              << " start=" << fixture.start
                              << " target=" << boundaryFixture.target
                              << " forward_settled=" << forward.work.settled
                              << " forward_arcs=" << forward.work.arcScans
                              << " forward_dominance=" << forward.work.dominanceChecks
                              << " bidirectional_settled=" << bidirectional.work.settled
                              << " bidirectional_arcs=" << bidirectional.work.arcScans
                              << " bidirectional_dominance=" << bidirectional.work.dominanceChecks
                              << " bidirectional_join=" << bidirectional.work.joinPairs
                              << '\n';
                    if (!sameFrontier) {
                        const auto referenceKeys = keys(reference);
                        const auto bidirectionalKeys = keys(bidirectional.frontier);
                        std::vector<std::tuple<std::uint32_t, std::int32_t, std::int32_t>> referenceOnly;
                        std::vector<std::tuple<std::uint32_t, std::int32_t, std::int32_t>> bidirectionalOnly;
                        std::set_difference(
                            referenceKeys.begin(),
                            referenceKeys.end(),
                            bidirectionalKeys.begin(),
                            bidirectionalKeys.end(),
                            std::back_inserter(referenceOnly));
                        std::set_difference(
                            bidirectionalKeys.begin(),
                            bidirectionalKeys.end(),
                            referenceKeys.begin(),
                            referenceKeys.end(),
                            std::back_inserter(bidirectionalOnly));
                        for (const auto& [mask, steps, fuel] : referenceOnly) {
                            std::cout << "reference_only mask=" << mask
                                      << " steps=" << steps
                                      << " fuel=" << fuel << '\n';
                        }
                        for (const auto& [mask, steps, fuel] : bidirectionalOnly) {
                            std::cout << "bidirectional_only mask=" << mask
                                      << " steps=" << steps
                                      << " fuel=" << fuel << '\n';
                        }
                    }
                }
            }
        }
        if (cases == 0) {
            throw std::runtime_error("no boundary case selected");
        }
        std::sort(ratios.begin(), ratios.end());
        const double median = ratios.size() % 2U == 1U
            ? ratios.at(ratios.size() / 2U)
            : (ratios.at(ratios.size() / 2U - 1U) + ratios.at(ratios.size() / 2U)) / 2.0;
        const bool pass = frontierMismatch == 0 && rankMismatch == 0 &&
            deterministicFailure == 0 && invalid == 0 && reverseCostFailure == 0;
        const bool gatePass = pass && median <= 0.80 && reducedFamilies.size() >= 3U;
        std::cout << "summary"
                  << " experiment=" << boundary::kBoundaryExperiment
                  << " split=" << options.split
                  << " cases=" << cases
                  << " frontier_mismatch=" << frontierMismatch
                  << " best_rank_mismatch=" << rankMismatch
                  << " deterministic_failure=" << deterministicFailure
                  << " invalid=" << invalid
                  << " reverse_cost_failure=" << reverseCostFailure
                  << " median_work_ratio=" << std::fixed << std::setprecision(6) << median
                  << " reduced_families=" << reducedFamilies.size()
                  << " parity_pass=" << (pass ? 1 : 0)
                  << " gate_pass=" << (gatePass ? 1 : 0)
                  << " result_hash=" << std::hex << resultHash << std::dec
                  << '\n';
        return gatePass ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "bidirectional_rcsp_boundary_probe error: " << error.what() << '\n';
        return 1;
    }
}
