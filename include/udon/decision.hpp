#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "udon/planner.hpp"

namespace udon {

struct RoadLoadInterval {
    std::int32_t minimum = 0;
    std::int32_t maximum = 0;
};

class TrafficBelief {
public:
    explicit TrafficBelief(const MatchConfig& config);

    void observe(const DayState& state);
    void record_own_footprint(std::int32_t dayNumber, const std::vector<std::int32_t>& footprint);

    [[nodiscard]] const std::vector<RoadLoadInterval>& opponent_carry() const;
    [[nodiscard]] const std::vector<std::int32_t>& previous_own_footprint() const;
    [[nodiscard]] const std::vector<std::vector<CellId>>& public_endpoint_history() const;
    [[nodiscard]] const std::vector<std::vector<OtherTeamState>>& public_opponent_history() const;

private:
    const MatchConfig& config_;
    std::vector<std::vector<std::int32_t>> ownHistory_;
    std::vector<RoadLoadInterval> opponentCarry_;
    std::vector<std::int32_t> previousOwnFootprint_;
    std::vector<std::vector<CellId>> publicEndpointHistory_;
    std::vector<std::vector<OtherTeamState>> publicOpponentHistory_;
    std::optional<std::int32_t> lastSubmittedDay;
    std::optional<std::int32_t> lastObservedDay;
};

struct TrafficScenario {
    std::int32_t scenarioId = 0;
    std::string scenarioClass;
    std::uint32_t weight = 0;
    bool adversarial = false;
    bool likely = false;
    bool privateSensitivity = false;
    bool pessimisticFallback = false;
    bool jointFeasible = false;
    std::string construction;
    std::vector<std::int32_t> opponentCurrentFootprint;
    std::vector<std::int32_t> opponentCarryFootprint;

    [[nodiscard]] friend bool operator==(
        const TrafficScenario& left,
        const TrafficScenario& right) = default;
};

struct ScenarioManifest {
    std::vector<TrafficScenario> scenarios;
    std::uint32_t totalWeight = 0;
    bool survivalSignatureEnabled = true;
    bool usedStaticFallback = true;
    bool requiredClassesCovered = true;
    std::uint32_t effectiveCoverageBasisPoints = 10000;
    std::uint32_t calibrationErrorBasisPoints = 10000;
    std::string version;
    std::uint64_t generatorSeed = 0;
    std::string evaluatorHash;
    std::string riskPolicyVersion;
    std::uint32_t confidenceBasisPoints = 0;
    std::int32_t safetySlack = 0;
    std::uint32_t resolutionBasisPoints = 0;
    std::array<std::uint32_t, 5> quantileBasisPoints{};
};

class ScenarioGenerator {
public:
    explicit ScenarioGenerator(const MatchConfig& config);

    [[nodiscard]] ScenarioManifest freeze_manifest(
        const DayState& state,
        const TrafficBelief& belief,
        std::int32_t maximumAdversarialScenarios = 6) const;

    [[nodiscard]] std::vector<TrafficScenario> private_sensitivity_scenarios(
        const DayState& state,
        const TrafficBelief& belief,
        const std::vector<CellId>& privateRoads,
        std::int32_t maximumScenarios = 6) const;

private:
    const MatchConfig& config_;
    std::vector<std::int32_t> roadIndexByCell_;
    std::vector<std::vector<std::int32_t>> componentsWithoutRoad_;
};

[[nodiscard]] std::vector<RoadStatus> predict_next_road_statuses(
    const MatchConfig& config,
    const TrafficBelief& belief,
    const TrafficScenario& scenario,
    const std::vector<std::int32_t>& ownCurrentFootprint);

struct FutureWitness {
    std::vector<DayPlan> futurePlans;
    OfficialScore score;
    bool certified = false;
    bool lowerBoundOnly = false;
};

struct ConditionalTierBounds {
    std::int32_t coverage = 0;
    std::int32_t lowerDailyDistinct = 0;
    std::int32_t upperDailyDistinct = 0;
    std::int32_t lowerServings = 0;
    std::int32_t upperServings = 0;
    bool witnessBacked = false;
};

struct ViabilityFrontierPoint {
    std::int32_t coverage = 0;
    std::int32_t riskRank = 0;
    OfficialScore lowerBound;
    OfficialScore upperBound;
    FutureWitness witness;
    std::int32_t scenarioId = -1;
    std::string scenarioClass;
};

struct ViabilityBounds {
    OfficialScore upperBound;
    OfficialScore lowerBound;
    OfficialScore pessimisticUpperBound;
    std::int32_t coverageCap = 0;
    std::int32_t coverageSafe = 0;
    std::int32_t confidenceCoverage = 0;
    std::vector<std::int32_t> optimisticLatestDayByBrand;
    std::vector<std::int32_t> latestSafeDayByBrand;
    std::vector<std::int32_t> slackByBrand;
    std::vector<std::int32_t> optimisticAgentDaySlotsByBrand;
    std::vector<std::int32_t> safeAgentDaySlotsByBrand;
    bool matchingRelaxationFeasible = true;
    bool deadlineReached = false;
    std::vector<MandatoryReservation> reservations;
    std::vector<ConditionalTierBounds> conditionalTiers;
    std::vector<ViabilityFrontierPoint> frontier;
};

class FastViabilityAnalyzer {
public:
    explicit FastViabilityAnalyzer(const MatchConfig& config);

    [[nodiscard]] ViabilityBounds analyze(
        const DayState& state,
        const MatchLedger& ledger,
        std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt) const;

private:
    const MatchConfig& config_;
};

struct ScenarioOutcome {
    OfficialScore score;
    FutureWitness witness;
};

struct CandidateProfile {
    std::vector<ScenarioOutcome> outcomes;
    std::int32_t coverageCap = 0;
    std::int32_t coverageSafe = 0;
    std::int32_t confidenceCoverage = 0;
    std::array<OfficialScore, 5> quantiles{};
    std::vector<std::int32_t> survivalSignature;
    OfficialScore certifiedLowerBound;
    OfficialScore provisionalLowerBound;
    OfficialScore validUpperBound;
    bool hasValidUpperBound = false;
    std::vector<OfficialScore> scenarioValidUpperBounds;
    bool provisional = false;
    std::vector<std::uint32_t> scenarioWeights;
    bool signatureEnabled = true;
    std::vector<ConditionalTierBounds> conditionalTiers;
    std::vector<ViabilityFrontierPoint> frontier;
};

struct RiskPolicy {
    std::string version = "static-v1";
    std::uint32_t confidenceBasisPoints = 8000;
    std::int32_t safetySlack = 1;
    std::uint32_t resolutionBasisPoints = 100;
    std::array<std::uint32_t, 5> quantileBasisPoints{9500, 8000, 5000, 2000, 500};
};

struct CandidateEvaluation {
    MasterCandidate candidate;
    CandidateProfile profile;
};

struct ProfileFinalizeDiagnostics {
    std::int32_t passes = 0;
    std::int32_t candidatesFinalized = 0;
    std::int32_t survivalTieGroups = 0;
    std::int32_t survivalProfilesRebuilt = 0;
    std::chrono::microseconds elapsed{};
};

class FutureWitnessRepairer {
public:
    FutureWitnessRepairer(
        const MatchConfig& config,
        const RouteColumnGenerator& generator,
        const RouteMaster& master,
        const ExactStepSimulator& simulator,
        const IndependentDayValidator& validator,
        std::int32_t harvestExtensionMode = 2);

    [[nodiscard]] CandidateProfile provisional_profile(
        const MasterCandidate& candidate,
        const DayState& currentState,
        const MatchLedger& ledger,
        const TrafficBelief& belief,
        const ScenarioManifest& manifest,
        OfficialScore validUpperBound,
        std::int32_t fixedOperationCap,
        std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt) const;

    void repair_profile(
        CandidateProfile& profile,
        const MasterCandidate& candidate,
        const DayState& currentState,
        const MatchLedger& ledger,
        const TrafficBelief& belief,
        const ScenarioManifest& manifest,
        std::int32_t routeCombinationCap,
        std::chrono::steady_clock::time_point deadline) const;

private:
    const MatchConfig& config_;
    const RouteColumnGenerator& generator_;
    const RouteMaster& master_;
    const ExactStepSimulator& simulator_;
    const IndependentDayValidator& validator_;
    std::int32_t harvestExtensionMode_ = 2;
};

class LexicographicRiskComparator {
public:
    explicit LexicographicRiskComparator(RiskPolicy policy);

    void finalize_profiles(
        std::vector<CandidateEvaluation>& evaluations,
        const ScenarioManifest& manifest,
        ProfileFinalizeDiagnostics* diagnostics = nullptr) const;

    [[nodiscard]] std::size_t choose(
        const std::vector<CandidateEvaluation>& evaluations,
        bool requireUndominatedCurrentFloor = false) const;
    [[nodiscard]] std::size_t choose_provisional(const std::vector<CandidateEvaluation>& evaluations) const;
    [[nodiscard]] bool certified_dominates(
        const CandidateProfile& challenger,
        const CandidateProfile& incumbent) const;
    [[nodiscard]] const RiskPolicy& policy() const;

private:
    RiskPolicy policy_;
};

enum class DeadlineClass : std::uint8_t {
    Emergency,
    Short,
    Normal,
    Long,
};

struct DeadlineProfile {
    DeadlineClass deadlineClass = DeadlineClass::Emergency;
    std::chrono::milliseconds total{};
    std::chrono::milliseconds seed{};
    std::chrono::milliseconds fastViability{};
    std::chrono::milliseconds search{};
    std::chrono::milliseconds searchSoft{};
    std::chrono::milliseconds certification{};
    std::chrono::milliseconds network{};
    bool meetsMinimumFloors = true;
    bool p99Calibrated = false;
    bool competitionReady = false;
    std::string calibrationVersion;
};

struct DeadlineCalibration {
    std::string version = "uncalibrated-v2";
    std::chrono::milliseconds seedFloor{30};
    std::chrono::milliseconds validationFloor{25};
    std::chrono::milliseconds networkFloor{50};
    std::int32_t seedPercent = 10;
    std::int32_t shortFastViabilityPercent = 5;
    std::int32_t normalFastViabilityPercent = 10;
    std::int32_t certificationPercent = 10;
    std::int32_t networkPercent = 10;
    std::int32_t shortSearchSoftPercent = 90;
    std::int32_t normalSearchSoftPercent = 90;
    std::int32_t longSearchSoftPercent = 90;
    std::chrono::milliseconds shortThreshold{500};
    std::chrono::milliseconds normalThreshold{5000};
    std::int32_t shortF0OperationCap = 8;
    std::int32_t normalF0OperationCap = 24;
    std::int32_t longF0OperationCap = 32;
    std::int32_t shortF0CandidateLimit = 8;
    std::int32_t normalF0CandidateLimit = 16;
    std::int32_t longF0CandidateLimit = 32;
    std::int32_t normalProofGuidedPasses = 2;
    std::int32_t longProofGuidedPasses = 4;
    std::int32_t f0SearchReservePercent = 25;
    std::chrono::milliseconds f0BoundaryGuard{150};
    bool p99Calibrated = false;
    bool requireCompetitionReadiness = false;
};

class DeadlineScheduler {
public:
    explicit DeadlineScheduler(DeadlineCalibration calibration = {});
    [[nodiscard]] DeadlineProfile classify(std::chrono::milliseconds available) const;
    [[nodiscard]] const DeadlineCalibration& calibration() const;
    [[nodiscard]] bool competition_ready() const;

private:
    DeadlineCalibration calibration_;
};

struct ResponseLedger {
    std::chrono::milliseconds totalResponse{};
    std::vector<std::optional<std::chrono::milliseconds>> lastValidResponseByDay;
    std::optional<CandidateProfile> lastProfile;
    std::optional<MasterCandidate> lastCandidate;
    std::optional<std::int32_t> lastProfileDay;
    struct CachedContingency {
        std::int32_t dayNumber = 0;
        std::int32_t scenarioId = 0;
        std::string scenarioClass;
        DayPlan plan;
        std::optional<FutureWitness> certifiedSuffix;
    };
    struct StrongProofRecord {
        std::int32_t dayNumber = 0;
        std::int32_t scenarioId = 0;
        std::string scenarioClass;
        std::string scope = "remaining-horizon-persistent-frozen-scenario-route-portfolios-v1";
        OfficialScore bestScore;
        OfficialScore upperBound;
        std::int32_t combinationsVisited = 0;
        std::int32_t branchesPruned = 0;
        bool complete = false;
        bool infeasible = false;
    };
    std::vector<CachedContingency> cachedContingencies;
    std::vector<StrongProofRecord> strongProofs;
};

struct CacheRepairDiagnostics {
    std::int32_t eligibleContingencies = 0;
    std::int32_t reusedContingencies = 0;
    std::int32_t rejectedContingencies = 0;
    bool deadlineReached = false;
};

struct CandidateAuditRecord {
    std::string stableId;
    OfficialScore scoreAfterToday;
    OfficialScore provisionalLowerBound;
    OfficialScore validUpperBound;
    OfficialScore finalQuantile50;
    OfficialScore finalCertifiedLowerBound;
    std::vector<CellId> terminalCells;
    std::vector<std::int32_t> terminalFuel;
    bool certified = false;
    bool selected = false;
    std::string w1Role;
    std::string disposition;
};

struct LexicographicGapDiagnostics {
    OfficialScore lowerBound;
    OfficialScore upperBound;
    std::array<std::int32_t, 3> componentGaps{};
    std::int32_t firstOpenTier = 0;
    bool validEnvelope = true;
};

struct OptimalityGapDiagnostics {
    LexicographicGapDiagnostics todayPortfolio;
    LexicographicGapDiagnostics candidateHorizon;
    LexicographicGapDiagnostics viabilityHorizon;
    LexicographicGapDiagnostics absoluteHorizon;
    bool portfolioSearchComplete = false;
    bool viabilityDeadlineReached = false;
};

struct DecisionAudit {
    std::string selectionReason;
    std::vector<CellId> promotedCriticalRoads;
    std::vector<CellId> privateSensitivityRoads;
    std::vector<std::int32_t> portfolioColumnsByAgent;
    std::vector<std::int32_t> portfolioBrandCountsByAgent;
    std::vector<std::int32_t> portfolioMaximumServingsByAgent;
    std::vector<std::int32_t> portfolioHarvestExtensionsByAgent;
    std::vector<std::vector<CellId>> portfolioTerminalCellsByAgent;
    ColumnGenerationDiagnostics columnGeneration;
    std::vector<CandidateAuditRecord> candidates;
    std::int32_t independentRoutesGenerated = 0;
    std::int32_t independentPlansEvaluated = 0;
    std::int32_t independentSearchNodes = 0;
    std::int32_t independentRollouts = 0;
    bool independentCandidateAccepted = false;
    bool independentDeadlineReached = false;
    ProfileFinalizeDiagnostics profileFinalization;
    OptimalityGapDiagnostics optimalityGap;
};

struct DecisionTiming {
    std::chrono::milliseconds incumbent{};
    std::chrono::milliseconds fastPath{};
    std::chrono::milliseconds columnGeneration{};
    std::chrono::milliseconds initialMaster{};
    std::chrono::milliseconds search{};
    std::chrono::milliseconds independentGenerators{};
    std::chrono::milliseconds candidatePreparation{};
    std::chrono::milliseconds certification{};
    std::chrono::milliseconds alns{};
    std::chrono::milliseconds recombination{};
    std::chrono::milliseconds total{};
};

struct DecisionResult {
    MasterCandidate candidate;
    CandidateProfile profile;
    ScenarioManifest manifest;
    ViabilityBounds viability;
    DeadlineProfile deadline;
    MasterDiagnostics diagnostics;
    AlnsDiagnostics alns;
    CacheRepairDiagnostics cacheRepair;
    DecisionAudit audit;
    DecisionTiming timing;
    RiskPolicy riskPolicy;
    std::int32_t dayNumber = 0;
    bool emergency = false;
    bool reconciledAuthoritativeState = false;
};

enum class RoutePoolSearch : std::uint8_t {
    SinglePass,
    Feedback,
};

[[nodiscard]] bool role_assignment_better_after_rollout(
    const RoleAssignment& challenger,
    const RoleAssignment& incumbent);

[[nodiscard]] std::int32_t role_comparison_beam_width(
    const MatchConfig& config,
    std::int32_t requestedWidth);

[[nodiscard]] bool apply_incomplete_long_horizon_role_fallback(
    const MatchConfig& config,
    bool fullHorizonComparisonComplete,
    std::vector<RoleAssignment>& beam,
    bool includeShortHorizon = false);

class UdonShieldEngine {
public:
    explicit UdonShieldEngine(
        const MatchConfig& config,
        RiskPolicy policy = {},
        DeadlineCalibration deadlineCalibration = {},
        RoutePoolSearch routePoolSearch = RoutePoolSearch::SinglePass,
        std::int32_t harvestExtensionMode = 6,
        bool requireUndominatedCurrentFloor = false,
        std::int32_t futureHarvestExtensionMode = 5);

    [[nodiscard]] std::vector<RoleAssignment> select_roles_exhaustive_oracle(
        std::int32_t beamWidth = 3) const;
    [[nodiscard]] RoleSelectionDiagnostics
    select_roles_exhaustive_oracle_with_diagnostics(
        std::int32_t beamWidth = 3) const;
    [[nodiscard]] std::vector<RoleAssignment> select_roles_until(
        std::chrono::milliseconds available,
        std::int32_t beamWidth = 3) const;
    [[nodiscard]] RoleSelectionDiagnostics select_roles_until_with_diagnostics(
        std::chrono::milliseconds available,
        std::int32_t beamWidth = 3) const;

    void set_short_horizon_role_fallback(bool enabled);

    [[nodiscard]] DecisionResult solve_day(
        const DayState& state,
        const MatchLedger& ledger,
        std::chrono::milliseconds available);

    [[nodiscard]] DecisionResult solve_day_until(
        const DayState& state,
        const MatchLedger& ledger,
        std::chrono::system_clock::time_point receivedAt = std::chrono::system_clock::now());

    void record_submitted(
        const DecisionResult& decision,
        std::chrono::milliseconds responseTime);

    void record_applied_transition(
        const DayState& state,
        const SimulationResult& simulation);

    void restore_response_artifacts(
        std::vector<ResponseLedger::CachedContingency> cachedContingencies,
        std::vector<ResponseLedger::StrongProofRecord> strongProofs);

    [[nodiscard]] std::int32_t precompute_next_day_contingencies(
        const DayState& state,
        const MatchLedger& ledger,
        const DecisionResult& decision,
        std::chrono::steady_clock::time_point deadline);

    [[nodiscard]] std::int32_t prove_remaining_horizon(
        const DayState& state,
        const MatchLedger& ledger,
        const DecisionResult& decision,
        std::chrono::steady_clock::time_point deadline);

    [[nodiscard]] bool may_submit(const DecisionResult& decision) const;
    [[nodiscard]] const ResponseLedger& response_ledger() const;
    [[nodiscard]] const std::vector<std::int32_t>& previous_own_footprint() const;
    [[nodiscard]] std::chrono::milliseconds remaining_post_ack_compute_budget() const;

private:
    template <bool CaptureDailyTrace>
    [[nodiscard]] std::vector<RoleAssignment>
    select_roles_exhaustive_oracle_impl(
        std::int32_t beamWidth,
        std::vector<std::vector<std::int32_t>>* alignedDailyTraces) const;

    template <bool CaptureDailyTrace>
    [[nodiscard]] std::vector<RoleAssignment> select_roles_until_impl(
        std::chrono::milliseconds available,
        std::int32_t beamWidth,
        std::vector<std::vector<std::int32_t>>* alignedDailyTraces) const;

    void rollout_role_assignment(
        RoleAssignment& assignment,
        std::int32_t maximumDays,
        std::int32_t maximumCombinationsPerDay,
        std::optional<std::chrono::steady_clock::time_point> deadline) const;

    template <bool CaptureDailyTrace>
    void rollout_role_assignment_impl(
        RoleAssignment& assignment,
        std::int32_t maximumDays,
        std::int32_t maximumCombinationsPerDay,
        std::optional<std::chrono::steady_clock::time_point> deadline,
        std::vector<std::int32_t>* dailyTrace) const;

    MatchConfig config_;
    ExactStepSimulator simulator_;
    IndependentDayValidator validator_;
    ParetoRouter router_;
    RouteColumnGenerator generator_;
    RouteMaster master_;
    AdaptiveRouteImprover alns_;
    GreedyPlanner greedy_;
    TrafficBelief belief_;
    ScenarioGenerator scenarioGenerator_;
    FastViabilityAnalyzer viabilityAnalyzer_;
    FutureWitnessRepairer witnessRepairer_;
    LexicographicRiskComparator comparator_;
    DeadlineScheduler deadlineScheduler_;
    RoutePoolSearch routePoolSearch_;
    std::int32_t harvestExtensionMode_ = 5;
    bool requireUndominatedCurrentFloor_ = false;
    bool shortHorizonRoleFallback_ = false;
    ResponseLedger ledger_;
    std::optional<std::int32_t> lastSubmittedDay_;
    std::optional<std::vector<AgentState>> expectedNextAgents_;
    std::optional<std::vector<std::int32_t>> previousOwnFootprintBeforeLastSubmission_;
    std::chrono::milliseconds remainingPostAckComputeBudget_{0};
};

} 
