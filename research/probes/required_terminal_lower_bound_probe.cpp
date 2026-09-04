#define main udonshield_frontier_297_frozen_main
#include "bidirectional_rcsp_probe.cpp"
#undef main

namespace goal {

using Rank = std::tuple<
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    std::uint32_t>;

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
    std::string preferredMode;
    std::int32_t minimumSpots = 0;
    std::size_t maximumRoutes = 0U;
    std::uint64_t firstSeed = 0;
    std::int32_t count = 0;
};

struct GoalOptions {
    std::string manifest =
        "research/holdouts/ATTR-REQUIRED-TERMINAL-LOWER-BOUND-300.csv";
    std::string split = "development";
    std::optional<std::uint64_t> onlySeed;
    bool details = false;
};

struct GoalFixture {
    Fixture base;
    udon::CellId target = udon::kInvalidCell;
    udon::BrandMask preferredBrands;
};

struct Choice {
    Rank rank;
    FrontierEntry entry;
};

struct GoalQueueItem {
    std::uint8_t cardinality = 0U;
    std::int32_t steps = 0;
    std::int32_t fuel = 0;
    std::int32_t label = -1;

    [[nodiscard]] friend bool operator>(
        const GoalQueueItem& left,
        const GoalQueueItem& right) {
        return std::tie(left.cardinality, left.steps, left.fuel, left.label) >
            std::tie(right.cardinality, right.steps, right.fuel, right.label);
    }
};

[[nodiscard]] GoalOptions parse_goal_options(std::int32_t argc, char** argv) {
    GoalOptions options;
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

[[nodiscard]] bool insert_goal_label(
    const udon::MatchConfig& config,
    SearchResult& result,
    Label candidate,
    std::priority_queue<
        GoalQueueItem,
        std::vector<GoalQueueItem>,
        std::greater<>>& queue) {
    std::vector<std::int32_t>& bucket = result.atState.at(static_cast<std::size_t>(
        state_index(config, candidate.mask, candidate.cell)));
    for (const std::int32_t labelIndex : bucket) {
        Label& existing = result.labels.at(static_cast<std::size_t>(labelIndex));
        if (!existing.active) {
            continue;
        }
        ++result.work.dominanceChecks;
        if (existing.steps <= candidate.steps && existing.fuel <= candidate.fuel) {
            return false;
        }
    }
    for (const std::int32_t labelIndex : bucket) {
        Label& existing = result.labels.at(static_cast<std::size_t>(labelIndex));
        if (!existing.active) {
            continue;
        }
        ++result.work.dominanceChecks;
        if (candidate.steps <= existing.steps && candidate.fuel <= existing.fuel) {
            existing.active = false;
        }
    }
    const std::int32_t index = static_cast<std::int32_t>(result.labels.size());
    result.labels.push_back(std::move(candidate));
    bucket.push_back(index);
    const Label& inserted = result.labels.back();
    queue.push(GoalQueueItem{
        static_cast<std::uint8_t>(32U - std::popcount(inserted.mask)),
        inserted.steps,
        inserted.fuel,
        index,
    });
    return true;
}

[[nodiscard]] GoalFixture make_goal_fixture(const Row& row, std::uint64_t seed) {
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
    GoalFixture fixture{make_fixture(baseRow, seed), udon::kInvalidCell, {}};
    for (std::size_t spot = 2U; spot < fixture.base.config.spots.size(); spot += 3U) {
        fixture.base.config.spots.at(spot).brandIndex =
            fixture.base.config.spots.at(spot - 1U).brandIndex;
    }
    if (row.startMode == "spot") {
        fixture.base.start = fixture.base.config.spots.front().position;
        fixture.base.state.agents.front().position = fixture.base.start;
    }
    if (row.targetMode == "spot") {
        fixture.target = fixture.base.config.spots.back().position;
    } else {
        const std::int32_t cells = fixture.base.config.map.cell_count();
        std::uint64_t cursor = seed ^ 0xe7037ed1a0b428dbULL;
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
        throw std::runtime_error("failed to choose goal terminal");
    }
    for (std::int32_t brand = 0; brand < fixture.base.config.brand_count(); ++brand) {
        const bool preferred = row.preferredMode == "all" ||
            (row.preferredMode == "alternating" && brand % 2 == 0) ||
            (row.preferredMode == "missing" &&
             brand >= fixture.base.config.brand_count() / 2);
        if (preferred) {
            fixture.preferredBrands |= udon::brand_bit(brand);
        }
    }
    return fixture;
}

[[nodiscard]] Rank route_rank(
    const GoalFixture& fixture,
    std::uint32_t mask,
    std::int32_t steps,
    std::int32_t fuel) {
    udon::BrandMask brands;
    std::int32_t servingPotential = 0;
    for (std::size_t spot = 0; spot < fixture.base.config.spots.size(); ++spot) {
        if ((mask & (std::uint32_t{1} << static_cast<std::uint32_t>(spot))) == 0U) {
            continue;
        }
        brands |= udon::brand_bit(fixture.base.config.spots.at(spot).brandIndex);
        servingPotential += fixture.base.config.spots.at(spot).stock > 0 ? 1 : 0;
    }
    std::int32_t terminalBrandDistance = 0;
    for (std::int32_t brand = 0; brand < fixture.base.config.brand_count(); ++brand) {
        std::int32_t nearest = std::numeric_limits<std::int32_t>::max();
        for (const udon::Spot& spot : fixture.base.config.spots) {
            if (spot.brandIndex == brand) {
                nearest = std::min(
                    nearest,
                    fixture.base.config.map.hex_distance(fixture.target, spot.position));
            }
        }
        if (nearest != std::numeric_limits<std::int32_t>::max()) {
            terminalBrandDistance += nearest;
        }
    }
    return Rank{
        udon::brand_intersection_count(brands, fixture.preferredBrands),
        udon::brand_count(brands),
        servingPotential,
        static_cast<std::int32_t>(std::popcount(mask)),
        -steps,
        -fuel,
        -terminalBrandDistance,
        -fixture.target,
        std::numeric_limits<std::uint32_t>::max() - mask,
    };
}

void retain_choice(std::vector<Choice>& choices, Choice candidate, std::size_t maximumRoutes) {
    if (choices.size() < maximumRoutes) {
        choices.push_back(std::move(candidate));
        return;
    }
    const auto worst = std::min_element(
        choices.begin(),
        choices.end(),
        [](const Choice& left, const Choice& right) { return left.rank < right.rank; });
    if (worst->rank < candidate.rank) {
        *worst = std::move(candidate);
    }
}

void sort_choices(std::vector<Choice>& choices) {
    std::sort(choices.begin(), choices.end(), [](const Choice& left, const Choice& right) {
        return left.rank > right.rank;
    });
}

[[nodiscard]] std::vector<std::tuple<Rank, std::uint32_t, std::int32_t, std::int32_t>>
choice_keys(const std::vector<Choice>& choices) {
    std::vector<std::tuple<Rank, std::uint32_t, std::int32_t, std::int32_t>> result;
    result.reserve(choices.size());
    for (const Choice& choice : choices) {
        result.emplace_back(
            choice.rank,
            choice.entry.mask,
            choice.entry.steps,
            choice.entry.fuel);
    }
    return result;
}

[[nodiscard]] bool validate_choice(
    const GoalFixture& fixture,
    const Choice& choice,
    std::string& mismatch) {
    udon::DayPlan plan;
    plan.actions.resize(static_cast<std::size_t>(fixture.base.config.agent_count()));
    for (const std::int32_t direction : choice.entry.directions) {
        plan.actions.front().push_back(
            direction == udon::kDirectionCount
                ? udon::PlanAction::wait(1)
                : udon::PlanAction::move(direction));
    }
    const std::int32_t remaining =
        fixture.base.config.steps_for_day(fixture.base.state.dayNumber) - choice.entry.steps;
    if (remaining > 0) {
        plan.actions.front().push_back(udon::PlanAction::wait(remaining));
    }
    for (std::int32_t agent = 1; agent < fixture.base.config.agent_count(); ++agent) {
        plan.actions.at(static_cast<std::size_t>(agent)).push_back(
            udon::PlanAction::wait(
                fixture.base.config.steps_for_day(fixture.base.state.dayNumber)));
    }
    const udon::ExactStepSimulator simulator(fixture.base.config);
    const udon::IndependentDayValidator validator(fixture.base.config);
    const udon::SimulationResult simulation = simulator.simulate(fixture.base.state, plan, false);
    const udon::SimulationResult independent = validator.validate(fixture.base.state, plan, false);
    if (!simulation.valid || !independent.valid ||
        !validator.agrees_with(simulation, independent, mismatch)) {
        return false;
    }
    std::uint32_t observed = 0U;
    for (const udon::ClaimEvent& claim : simulation.claims) {
        if (claim.agent == 0 && claim.served) {
            observed |= std::uint32_t{1} << static_cast<std::uint32_t>(claim.spot);
        }
    }
    if (observed != choice.entry.mask ||
        simulation.finalAgents.front().position != fixture.target ||
        simulation.finalAgents.front().fuel !=
            fixture.base.config.fuelLimit - choice.entry.fuel) {
        mismatch = "validated route tuple differs";
        return false;
    }
    return true;
}

[[nodiscard]] std::uint64_t choices_hash(const std::vector<Choice>& choices) {
    std::uint64_t hash = kFnvOffset;
    for (const Choice& choice : choices) {
        hash_value(hash, choice.entry.mask);
        hash_value(hash, static_cast<std::uint64_t>(choice.entry.steps));
        hash_value(hash, static_cast<std::uint64_t>(choice.entry.fuel));
    }
    return hash;
}

}  // namespace goal

namespace terminal_bound {

constexpr const char* kExperiment = "ATTR-REQUIRED-TERMINAL-LOWER-BOUND-300";

[[nodiscard]] std::vector<goal::Row> load_manifest(
    const std::string& path,
    const std::string& requestedSplit) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open manifest: " + path);
    }
    std::string line;
    constexpr const char* header =
        "experiment_id,split,family,width,height,agent_count,spot_count,players,day_steps,fuel_profile,start_mode,target_mode,preferred_mode,minimum_spots,maximum_routes,first_seed,count,scope";
    if (!std::getline(input, line) || line != header) {
        throw std::runtime_error("unexpected terminal-bound manifest schema");
    }
    std::vector<goal::Row> rows;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split_csv(line);
        if (fields.size() != 18U || fields.at(0) != kExperiment ||
            fields.at(17) != "exact-required-terminal-topk") {
            throw std::runtime_error("invalid terminal-bound manifest row: " + line);
        }
        if (fields.at(1) != requestedSplit) {
            continue;
        }
        goal::Row row;
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
        row.preferredMode = fields.at(12);
        row.minimumSpots = std::stoi(fields.at(13));
        row.maximumRoutes = static_cast<std::size_t>(std::stoul(fields.at(14)));
        row.firstSeed = std::stoull(fields.at(15));
        row.count = std::stoi(fields.at(16));
        if ((row.startMode != "spot" && row.startMode != "nonspot") ||
            (row.targetMode != "spot" && row.targetMode != "nonspot") ||
            (row.preferredMode != "all" && row.preferredMode != "missing" &&
             row.preferredMode != "alternating") ||
            row.spotCount > 12 || row.minimumSpots <= 0 ||
            row.minimumSpots > row.spotCount || row.maximumRoutes == 0U || row.count <= 0) {
            throw std::runtime_error("terminal-bound row violates scope: " + line);
        }
        rows.push_back(std::move(row));
    }
    if (rows.empty()) {
        throw std::runtime_error("manifest contains no rows for split " + requestedSplit);
    }
    return rows;
}

[[nodiscard]] std::vector<goal::Choice> unbounded_choices(
    const goal::GoalFixture& fixture,
    const goal::Row& row,
    SearchResult& search) {
    const Fixture& base = fixture.base;
    search.atState.resize(
        (std::size_t{1} << base.config.spots.size()) *
        static_cast<std::size_t>(base.config.map.cell_count()));
    std::priority_queue<
        goal::GoalQueueItem,
        std::vector<goal::GoalQueueItem>,
        std::greater<>> queue;
    static_cast<void>(goal::insert_goal_label(
        base.config,
        search,
        Label{base.start, 0U},
        queue));
    const std::uint32_t startBit = mask_at(base.config, base.start);
    if (startBit != 0U) {
        static_cast<void>(goal::insert_goal_label(
            base.config,
            search,
            Label{base.start, startBit, 1, 0, 0, udon::kDirectionCount, true},
            queue));
    }
    std::vector<goal::Choice> choices;
    std::unordered_set<std::uint32_t> emittedMasks;
    while (!queue.empty()) {
        const goal::GoalQueueItem item = queue.top();
        queue.pop();
        const Label label = search.labels.at(static_cast<std::size_t>(item.label));
        if (!label.active) {
            continue;
        }
        ++search.work.settled;
        if (label.cell == fixture.target &&
            static_cast<std::int32_t>(std::popcount(label.mask)) >= row.minimumSpots &&
            emittedMasks.insert(label.mask).second) {
            ++search.work.dominanceChecks;
            goal::retain_choice(choices, goal::Choice{
                goal::route_rank(fixture, label.mask, label.steps, label.fuel),
                FrontierEntry{
                    label.mask,
                    label.steps,
                    label.fuel,
                    reconstruct_forward(search, item.label),
                },
            }, row.maximumRoutes);
        }
        const udon::MoveCost cost = base.config.move_cost(
            label.cell,
            base.state.roadStatuses.at(static_cast<std::size_t>(label.cell)));
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            ++search.work.arcScans;
            const udon::CellId next = base.config.map.neighbors
                .at(static_cast<std::size_t>(label.cell))
                .at(static_cast<std::size_t>(direction));
            if (next == udon::kInvalidCell ||
                base.config.map.terrain.at(static_cast<std::size_t>(next)) ==
                    udon::Terrain::Pond) {
                continue;
            }
            const std::int32_t steps = label.steps + cost.steps;
            const std::int32_t fuel = label.fuel + cost.patrolFuel;
            if (steps > row.daySteps || fuel > base.config.fuelLimit) {
                continue;
            }
            static_cast<void>(goal::insert_goal_label(
                base.config,
                search,
                Label{
                    next,
                    label.mask | mask_at(base.config, next),
                    steps,
                    fuel,
                    item.label,
                    direction,
                    true,
                },
                queue));
        }
    }
    goal::sort_choices(choices);
    return choices;
}

[[nodiscard]] std::vector<goal::Choice> bounded_choices(
    const goal::GoalFixture& fixture,
    const goal::Row& row,
    SearchResult& search) {
    const Fixture& base = fixture.base;
    search.atState.resize(
        (std::size_t{1} << base.config.spots.size()) *
        static_cast<std::size_t>(base.config.map.cell_count()));
    std::priority_queue<
        goal::GoalQueueItem,
        std::vector<goal::GoalQueueItem>,
        std::greater<>> queue;
    const TargetBounds bounds = target_bounds(base, fixture.target);
    search.work += bounds.work;
    const auto can_reach_target = [&](const Label& label) {
        return bounds.steps.at(static_cast<std::size_t>(label.cell)) <=
                row.daySteps - label.steps &&
            bounds.fuel.at(static_cast<std::size_t>(label.cell)) <=
                base.config.fuelLimit - label.fuel;
    };
    const Label root{base.start, 0U};
    if (can_reach_target(root)) {
        static_cast<void>(goal::insert_goal_label(base.config, search, root, queue));
    }
    const std::uint32_t startBit = mask_at(base.config, base.start);
    const Label initialClaim{
        base.start,
        startBit,
        1,
        0,
        0,
        udon::kDirectionCount,
        true,
    };
    if (startBit != 0U && can_reach_target(initialClaim)) {
        static_cast<void>(goal::insert_goal_label(
            base.config,
            search,
            initialClaim,
            queue));
    }
    std::vector<goal::Choice> choices;
    std::unordered_set<std::uint32_t> emittedMasks;
    while (!queue.empty()) {
        const goal::GoalQueueItem item = queue.top();
        queue.pop();
        const Label label = search.labels.at(static_cast<std::size_t>(item.label));
        if (!label.active) {
            continue;
        }
        ++search.work.settled;
        if (label.cell == fixture.target &&
            static_cast<std::int32_t>(std::popcount(label.mask)) >= row.minimumSpots &&
            emittedMasks.insert(label.mask).second) {
            ++search.work.dominanceChecks;
            goal::retain_choice(choices, goal::Choice{
                goal::route_rank(fixture, label.mask, label.steps, label.fuel),
                FrontierEntry{
                    label.mask,
                    label.steps,
                    label.fuel,
                    reconstruct_forward(search, item.label),
                },
            }, row.maximumRoutes);
        }
        const udon::MoveCost cost = base.config.move_cost(
            label.cell,
            base.state.roadStatuses.at(static_cast<std::size_t>(label.cell)));
        for (std::int32_t direction = 0; direction < udon::kDirectionCount; ++direction) {
            ++search.work.arcScans;
            const udon::CellId next = base.config.map.neighbors
                .at(static_cast<std::size_t>(label.cell))
                .at(static_cast<std::size_t>(direction));
            if (next == udon::kInvalidCell ||
                base.config.map.terrain.at(static_cast<std::size_t>(next)) ==
                    udon::Terrain::Pond) {
                continue;
            }
            const Label candidate{
                next,
                label.mask | mask_at(base.config, next),
                label.steps + cost.steps,
                label.fuel + cost.patrolFuel,
                item.label,
                direction,
                true,
            };
            if (candidate.steps > row.daySteps ||
                candidate.fuel > base.config.fuelLimit ||
                !can_reach_target(candidate)) {
                continue;
            }
            static_cast<void>(goal::insert_goal_label(
                base.config,
                search,
                candidate,
                queue));
        }
    }
    goal::sort_choices(choices);
    return choices;
}

}  // namespace terminal_bound

int main(std::int32_t argc, char** argv) {
    try {
        goal::GoalOptions options = goal::parse_goal_options(argc, argv);
        if (options.manifest ==
            "research/holdouts/ATTR-INCUMBENT-GOAL-RCSP-PRUNING-299.csv") {
            options.manifest =
                "research/holdouts/ATTR-REQUIRED-TERMINAL-LOWER-BOUND-300.csv";
        }
        const std::vector<goal::Row> rows =
            terminal_bound::load_manifest(options.manifest, options.split);
        std::int32_t cases = 0;
        std::int32_t topKMismatch = 0;
        std::int32_t deterministicFailure = 0;
        std::int32_t invalid = 0;
        std::vector<double> ratios;
        std::set<std::string> reducedFamilies;
        std::uint64_t resultHash = kFnvOffset;
        for (const goal::Row& row : rows) {
            for (std::int32_t offset = 0; offset < row.count; ++offset) {
                const std::uint64_t seed = row.firstSeed + static_cast<std::uint64_t>(offset);
                if (options.onlySeed.has_value() && *options.onlySeed != seed) {
                    continue;
                }
                const goal::GoalFixture fixture = goal::make_goal_fixture(row, seed);
                SearchResult baselineSearch;
                const std::vector<goal::Choice> baseline =
                    terminal_bound::unbounded_choices(fixture, row, baselineSearch);
                SearchResult boundedSearch;
                const std::vector<goal::Choice> bounded =
                    terminal_bound::bounded_choices(fixture, row, boundedSearch);
                SearchResult repeatedSearch;
                const std::vector<goal::Choice> repeated =
                    terminal_bound::bounded_choices(fixture, row, repeatedSearch);
                const bool same = goal::choice_keys(baseline) == goal::choice_keys(bounded);
                const bool deterministic =
                    goal::choice_keys(bounded) == goal::choice_keys(repeated) &&
                    boundedSearch.work.total() == repeatedSearch.work.total();
                std::int32_t caseInvalid = 0;
                for (const goal::Choice& choice : baseline) {
                    std::string mismatch;
                    if (!goal::validate_choice(fixture, choice, mismatch)) {
                        ++caseInvalid;
                    }
                }
                for (const goal::Choice& choice : bounded) {
                    std::string mismatch;
                    if (!goal::validate_choice(fixture, choice, mismatch)) {
                        ++caseInvalid;
                    }
                }
                const double ratio = baselineSearch.work.total() == 0U
                    ? 1.0
                    : static_cast<double>(boundedSearch.work.total()) /
                        static_cast<double>(baselineSearch.work.total());
                if (ratio < 1.0) {
                    reducedFamilies.insert(row.family);
                }
                ratios.push_back(ratio);
                ++cases;
                topKMismatch += same ? 0 : 1;
                deterministicFailure += deterministic ? 0 : 1;
                invalid += caseInvalid;
                hash_value(resultHash, seed);
                hash_value(resultHash, goal::choices_hash(baseline));
                hash_value(resultHash, goal::choices_hash(bounded));
                hash_value(resultHash, baselineSearch.work.total());
                hash_value(resultHash, boundedSearch.work.total());
                std::cout << "case"
                          << " split=" << options.split
                          << " family=" << row.family
                          << " seed=" << seed
                          << " top_k=" << baseline.size()
                          << " minimum_spots=" << row.minimumSpots
                          << " baseline_work=" << baselineSearch.work.total()
                          << " bounded_work=" << boundedSearch.work.total()
                          << " work_ratio=" << std::fixed << std::setprecision(6) << ratio
                          << " top_k_mismatch=" << (same ? 0 : 1)
                          << " deterministic_failure=" << (deterministic ? 0 : 1)
                          << " invalid=" << caseInvalid
                          << '\n';
                if (options.details && !same) {
                    std::cout << "detail seed=" << seed
                              << " baseline_hash=" << std::hex << goal::choices_hash(baseline)
                              << " bounded_hash=" << goal::choices_hash(bounded) << std::dec
                              << " baseline_count=" << baseline.size()
                              << " bounded_count=" << bounded.size()
                              << '\n';
                }
            }
        }
        if (cases == 0) {
            throw std::runtime_error("no terminal-bound case selected");
        }
        std::sort(ratios.begin(), ratios.end());
        const double median = ratios.size() % 2U == 1U
            ? ratios.at(ratios.size() / 2U)
            : (ratios.at(ratios.size() / 2U - 1U) + ratios.at(ratios.size() / 2U)) / 2.0;
        const bool parityPass =
            topKMismatch == 0 && invalid == 0 && deterministicFailure == 0;
        const bool gatePass =
            parityPass && median <= 0.80 && reducedFamilies.size() >= 3U;
        std::cout << "summary"
                  << " experiment=" << terminal_bound::kExperiment
                  << " split=" << options.split
                  << " cases=" << cases
                  << " top_k_mismatch=" << topKMismatch
                  << " deterministic_failure=" << deterministicFailure
                  << " invalid=" << invalid
                  << " median_work_ratio=" << std::fixed << std::setprecision(6) << median
                  << " reduced_families=" << reducedFamilies.size()
                  << " parity_pass=" << (parityPass ? 1 : 0)
                  << " gate_pass=" << (gatePass ? 1 : 0)
                  << " result_hash=" << std::hex << resultHash << std::dec
                  << '\n';
        return gatePass ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "required_terminal_lower_bound_probe error: " << error.what() << '\n';
        return 1;
    }
}
