#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "udon/graph.hpp"
#include "udon/simulator.hpp"
#include "udon/validator.hpp"

namespace udon {

struct ColumnVisitEvent {
    SpotIndex spot = kInvalidSpot;
    std::int32_t step = 0;
    bool claimedServing = true;
    std::int32_t brandIndex = -1;
    bool creditedServing = false;

    [[nodiscard]] friend bool operator==(const ColumnVisitEvent& left, const ColumnVisitEvent& right) = default;
};

struct RefuelEvent {
    CellId cell = kInvalidCell;
    std::int32_t step = 0;

    [[nodiscard]] friend bool operator==(const RefuelEvent& left, const RefuelEvent& right) = default;
};

struct RouteStepState {
    CellId position = kInvalidCell;
    std::int32_t fuel = 0;

    [[nodiscard]] friend bool operator==(const RouteStepState& left, const RouteStepState& right) = default;
};

struct EscortSegment {
    std::int32_t group = -1;
    std::int32_t firstStep = 0;
    std::int32_t lastStep = 0;
    std::vector<CellId> positions;

    [[nodiscard]] friend bool operator==(const EscortSegment& left, const EscortSegment& right) = default;
};

struct RouteTerminalFeatures {
    SpotIndex spot = kInvalidSpot;
    std::int32_t nearestUncollectedBrandSteps = std::numeric_limits<std::int32_t>::max();
    bool overnightHarvestCandidate = false;
    bool endStepDockRequired = false;
};

struct RouteColumn {
    std::int32_t columnId = -1;
    AgentIndex agent = kInvalidAgent;
    AgentPlan actions;
    CellId terminalCell = kInvalidCell;
    std::int32_t terminalFuel = 0;
    BrandMask estimatedBrands;
    std::int32_t estimatedServings = 0;
    SparseRoadFootprint heuristicFootprint;
    SparseRoadFootprint fullFootprint;
    std::vector<ColumnVisitEvent> firstVisits;
    std::vector<RefuelEvent> requiredRefuels;
    std::vector<RefuelEvent> providedRefuels;
    std::vector<RouteStepState> timeline;
    std::vector<EscortSegment> escortSegments;
    RouteTerminalFeatures terminalFeatures;
    bool hasExactTimeline = false;
    bool harvestExtension = false;
    bool exactOrienteering = false;
    std::int32_t harvestExtensionSourceRank = 0;
    std::int32_t escortGroup = -1;
    std::int32_t contingencyBundle = -1;
    bool lockstepEscort = false;
    std::int32_t priority = 0;
};

struct RoutePortfolio {
    std::vector<std::vector<RouteColumn>> columnsByAgent;
};

struct RoutePoolAugmentation {
    RoutePortfolio portfolio;
    std::int32_t routesConsidered = 0;
    std::int32_t novelRoutes = 0;
    std::int32_t retainedNovelRoutes = 0;
};

enum class ReservationEvidence : std::uint8_t {
    RelaxationOnly,
    WitnessBacked,
    Proven,
};

struct MandatoryReservation {
    std::int32_t brandIndex = -1;
    SpotIndex representativeSpot = kInvalidSpot;
    std::int32_t latestSafeDay = 0;
    bool proven = false;
    ReservationEvidence evidence = ReservationEvidence::RelaxationOnly;
};

[[nodiscard]] inline bool is_proven_reservation(const MandatoryReservation& reservation) {
    return reservation.proven || reservation.evidence == ReservationEvidence::Proven;
}

struct ColumnGenerationOptions {
    std::int32_t maximumPathsPerTarget = 4;
    std::int32_t maximumColumnsPerAgent = 8;
    std::int32_t maximumTargetSpots = 12;
    std::int32_t maximumEscorts = 16;
    std::int32_t maximumSeedPlans = 2;
    std::vector<MandatoryReservation> mandatoryReservations;
    std::vector<DayPlan> seedPlans;
    std::vector<CellId> criticalRoadHints;
    bool enableHarvestExtensions = true;
    bool allowUncachedHarvestTargets = true;
    bool enableHarvestOrienteering = false;
    bool enableExactHarvestOrienteering = false;
    bool enableFuelConstrainedExactHarvestOrienteering = false;
    bool enableAnytimeFuelConstrainedHarvestOrienteering = false;
    // Research option for ATTR-COORDINATED-EXACT-BUNDLE-FRONTIER-310.
    // One preserves the canonical coordinated bundle path exactly.  Larger
    // values may only append bounded equal-current-score resource-frontier
    // bundles; they never replace the canonical first bundle.
    std::int32_t maximumCoordinatedExactBundles = 1;
    std::int32_t maximumHarvestExtensionSources = 1;
    std::int32_t maximumHarvestExtensionDepth = 2;
    std::optional<std::chrono::steady_clock::time_point> deadline;
};

struct ColumnGenerationDiagnostics {
    ParetoSearchDiagnostics pareto;
    std::vector<CellId> criticalRoads;
    std::vector<std::int64_t> agentMilliseconds;
    std::vector<std::int32_t> agentParetoQueries;
    std::int64_t coordinationMilliseconds = 0;
    std::int32_t coordinationParetoQueries = 0;
    std::int32_t exactOrienteeringSupportedAgents = 0;
    std::int32_t exactOrienteeringCompleteAgents = 0;
    std::int32_t exactOrienteeringCacheHits = 0;
    std::uint64_t exactOrienteeringSettledStates = 0;
    std::uint64_t exactOrienteeringTerminalVariants = 0;
    std::int32_t exactOrienteeringBundles = 0;
    std::int32_t exactOrienteeringFrontierCandidates = 0;
    std::int32_t exactOrienteeringFrontierBundles = 0;
    std::int32_t exactOrienteeringSeedServings = 0;
    std::int32_t exactOrienteeringLocalServings = 0;
    std::uint64_t exactOrienteeringFeasibilityNodes = 0;
    std::uint64_t exactOrienteeringOverlapFeasibilityNodes = 0;
    std::int32_t exactOrienteeringFeasibilityImprovements = 0;
    bool exactOrienteeringFeasibilityImproved = false;
    bool exactOrienteeringOverlapFeasibilityImproved = false;
    std::int64_t exactOrienteeringMilliseconds = 0;
    std::int64_t exactOrienteeringEnumerationMilliseconds = 0;
    std::int64_t exactOrienteeringFinalizationMilliseconds = 0;
    std::int64_t exactOrienteeringDeadlineRemainingAtStartMilliseconds = -1;
    std::int64_t exactOrienteeringDeadlineOverrunMilliseconds = 0;
    bool deadlineReached = false;
};

struct MasterDiagnostics {
    std::int32_t combinationsVisited = 0;
    std::int32_t beamCombinationsVisited = 0;
    std::int32_t depthFirstCombinationsVisited = 0;
    std::int32_t branchOrderingCalls = 0;
    std::int32_t upperBoundChecks = 0;
    std::int32_t upperBoundPrunes = 0;
    std::int32_t bundleUpperBoundChecks = 0;
    std::int32_t bundleUpperBoundPrunes = 0;
    std::int32_t bundleBrandFrontierStates = 0;
    std::int32_t bundleBrandFrontierFallbacks = 0;
    std::int32_t bundlePrunes = 0;
    std::int32_t exactBundlesDiscovered = 0;
    std::int32_t exactBundlesEvaluated = 0;
    std::int32_t exactBundlesAccepted = 0;
    std::int32_t partialSynchronizationChecks = 0;
    std::int32_t partialSynchronizationPrunes = 0;
    std::int32_t simulatorValidCombinations = 0;
    std::int32_t branchesPruned = 0;
    std::int32_t stockCapacityConflicts = 0;
    std::int32_t prefixConflicts = 0;
    std::int32_t hotspotPromotions = 0;
    std::int32_t capCuts = 0;
    std::int32_t prefixCuts = 0;
    std::int32_t cutRounds = 0;
    std::int32_t stockCreditDenials = 0;
    std::int32_t exactCreditMismatches = 0;
    std::int32_t criticalRoadPromotions = 0;
    std::int32_t synchronizationConflicts = 0;
    std::int32_t duplicatePlansSkipped = 0;
    std::int32_t invalidPlanCombinations = 0;
    std::int32_t reservationConflicts = 0;
    std::int64_t roundPreparationMicroseconds = 0;
    std::int64_t beamConstructionMicroseconds = 0;
    std::int64_t beamEvaluationMicroseconds = 0;
    std::int64_t depthFirstSearchMicroseconds = 0;
    std::int64_t populationMaintenanceMicroseconds = 0;
    bool nativeExactStockCredits = false;
    bool stockCappedSearchOrder = false;
    bool bundleAwareUpperBound = false;
    bool deadlineReached = false;
    bool searchComplete = false;
    OfficialScore bestExactBundleScore;
    OfficialScore optimisticUpperBound;
    OfficialScore searchGuidanceUpperBound;
};

struct TrafficSafety {
    std::int32_t thresholdCrossings = 0;
    std::int32_t thresholdBandRoads = 0;
    std::int32_t totalRoadStays = 0;

    [[nodiscard]] friend bool operator==(const TrafficSafety& left, const TrafficSafety& right) = default;
};

struct TerminalSlack {
    std::int32_t worstRemainingBrandSteps = 0;
    std::int32_t totalRemainingBrandSteps = 0;
    std::int64_t patrolFuelReserve = 0;
    std::int32_t overnightSpotCount = 0;

    [[nodiscard]] friend bool operator==(const TerminalSlack& left, const TerminalSlack& right) = default;
};

[[nodiscard]] inline std::int32_t compare_terminal_slack(const TerminalSlack& left, const TerminalSlack& right) {
    if (left.worstRemainingBrandSteps != right.worstRemainingBrandSteps) {
        return left.worstRemainingBrandSteps < right.worstRemainingBrandSteps ? 1 : -1;
    }
    if (left.totalRemainingBrandSteps != right.totalRemainingBrandSteps) {
        return left.totalRemainingBrandSteps < right.totalRemainingBrandSteps ? 1 : -1;
    }
    if (left.patrolFuelReserve != right.patrolFuelReserve) {
        return left.patrolFuelReserve > right.patrolFuelReserve ? 1 : -1;
    }
    if (left.overnightSpotCount != right.overnightSpotCount) {
        return left.overnightSpotCount > right.overnightSpotCount ? 1 : -1;
    }
    return 0;
}

struct MasterCandidate {
    DayPlan plan;
    SimulationResult simulation;
    OfficialScore scoreAfterToday;
    TerminalSlack terminalSlack;
    TrafficSafety trafficSafety;
    std::string stableId;
    std::int32_t creditedServings = 0;
};

struct MasterOptions {
    std::int32_t maximumCombinations = 40000;
    std::int32_t maximumCandidates = 32;
    std::int32_t diversityCandidates = 0;
    std::int32_t maximumResolveRounds = 0;
    std::vector<MandatoryReservation> mandatoryReservations;
    bool useStockCredits = true;
    bool preferStockCappedSearchOrder = true;
    bool preferBaselineHarvestSources = false;
    bool enableLexicographicBranchAndBound = true;
    bool enableBundleAwareUpperBound = true;
    std::optional<std::chrono::steady_clock::time_point> deadline;
};

class RouteColumnGenerator {
public:
    RouteColumnGenerator(const MatchConfig& config, const ParetoRouter& router);

    [[nodiscard]] RoutePortfolio generate(
        const DayState& state,
        const MatchLedger& ledger,
        const ColumnGenerationOptions& options,
        ColumnGenerationDiagnostics* diagnostics = nullptr) const;

    [[nodiscard]] RoutePoolAugmentation augment_with_candidate_routes(
        const DayState& state,
        RoutePortfolio portfolio,
        const std::vector<MasterCandidate>& candidates,
        std::int32_t maximumColumnsPerAgent) const;

private:
    const MatchConfig& config_;
    const ParetoRouter& router_;
    std::vector<std::vector<std::int32_t>> terminalDistancesToSpots_;
};

class RouteMaster {
public:
    RouteMaster(
        const MatchConfig& config,
        const ExactStepSimulator& simulator,
        const IndependentDayValidator& validator);

    [[nodiscard]] std::vector<MasterCandidate> solve(
        const DayState& state,
        const MatchLedger& ledger,
        const RoutePortfolio& portfolio,
        const MasterOptions& options,
        MasterDiagnostics& diagnostics) const;

    [[nodiscard]] std::optional<MasterCandidate> evaluate_exact_plan(
        const DayState& state,
        const MatchLedger& ledger,
        const DayPlan& plan,
        const std::vector<MandatoryReservation>& mandatoryReservations = {}) const;

private:
    const MatchConfig& config_;
    const ExactStepSimulator& simulator_;
    const IndependentDayValidator& validator_;
    std::vector<std::vector<std::int32_t>> terminalDistancesToSpots_;
};

enum class AlnsOperator : std::uint8_t {
    RareBrandRescue,
    OvernightHarvest,
    DockOrRendezvous,
    MergeSplit,
    StockMultiVisit,
    CriticalRoadBypass,
    TerminalShift,
    ViabilityRepair,
};

struct AlnsDiagnostics {
    std::int32_t iterations = 0;
    std::int32_t accepted = 0;
    std::int32_t improvements = 0;
    std::int32_t synthesizedRoutes = 0;
    std::int32_t synthesizedAccepted = 0;
    std::int32_t proofGuidedIterations = 0;
    std::int32_t proofGuidedRoutes = 0;
    std::int32_t proofGuidedAccepted = 0;
    std::int32_t proofGuidedImprovements = 0;
    std::int32_t poolRoutesConsidered = 0;
    std::int32_t poolNovelRoutes = 0;
    std::int32_t poolRetainedRoutes = 0;
    std::int32_t recombinationCandidates = 0;
    std::int32_t recombinationImprovements = 0;
    bool recombinationDeadlineSkipped = false;
    std::array<std::int32_t, 8> attemptedByOperator{};
    std::array<std::int32_t, 8> acceptedByOperator{};
};

struct AlnsOptions {
    std::int32_t maximumIterations = 0;
    std::int32_t maximumProofGuidedIterations = 0;
    std::int32_t maximumAlternativesPerIteration = 4;
    std::int32_t maximumCandidates = 32;
    std::int32_t diversityCandidates = 0;
    OfficialScore proofUpperBound;
    std::vector<MandatoryReservation> mandatoryReservations;
    std::vector<CellId> criticalRoads;
    std::vector<std::int32_t> brandSlack;
    std::vector<std::int32_t> latestSafeDayByBrand;
    std::optional<std::chrono::steady_clock::time_point> deadline;
};

class AdaptiveRouteImprover {
public:
    AdaptiveRouteImprover(const MatchConfig& config, const RouteMaster& master);

    [[nodiscard]] std::vector<MasterCandidate> improve(
        const DayState& state,
        const MatchLedger& ledger,
        const RoutePortfolio& portfolio,
        std::vector<MasterCandidate> candidates,
        const AlnsOptions& options,
        AlnsDiagnostics& diagnostics) const;

private:
    const MatchConfig& config_;
    const RouteMaster& master_;
    ParetoRouter repairRouter_;
};

struct RoleAssignment {
    std::vector<AgentKind> roles;
    OfficialScore cheapUpperBound;
    OfficialScore rolloutScore;
    std::int32_t patrolCount = 0;
    std::int32_t sustainableCoverage = 0;
    bool rolloutValid = false;
    bool rolloutComplete = false;
};

struct RoleSelectionDiagnostics {
    std::vector<RoleAssignment> assignments;
    // Aligned with assignments; populated only by the explicit diagnostic APIs.
    std::vector<std::vector<std::int32_t>> rolloutDailyDistinct;
};

class RoleAssignmentEnumerator {
public:
    explicit RoleAssignmentEnumerator(const MatchConfig& config);

    [[nodiscard]] std::vector<RoleAssignment> shortlist(
        std::int32_t beamWidth,
        std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt) const;

private:
    const MatchConfig& config_;
};

class GreedyPlanner {
public:
    GreedyPlanner(
        const MatchConfig& config,
        const RouteColumnGenerator& generator,
        const RouteMaster& master);

    [[nodiscard]] MasterCandidate build_incumbent(
        const DayState& state,
        const MatchLedger& ledger,
        const ColumnGenerationOptions& generationOptions,
        const MasterOptions& masterOptions,
        MasterDiagnostics& diagnostics) const;

private:
    const MatchConfig& config_;
    const RouteColumnGenerator& generator_;
    const RouteMaster& master_;
};

[[nodiscard]] DayPlan emergency_wait_plan(const MatchConfig& config, const DayState& state);

} 
