#include "udon/audit.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

#include "udon/protocol.hpp"

namespace udon {

namespace {

template <typename Value>
[[nodiscard]] JsonValue integer_array(const std::vector<Value>& values) {
    JsonValue::Array array;
    array.reserve(values.size());
    for (const Value value : values) {
        array.emplace_back(static_cast<std::int64_t>(value));
    }
    return JsonValue(std::move(array));
}

template <typename Value, std::size_t Count>
[[nodiscard]] JsonValue integer_array(const std::array<Value, Count>& values) {
    JsonValue::Array array;
    array.reserve(Count);
    for (const Value value : values) {
        array.emplace_back(static_cast<std::int64_t>(value));
    }
    return JsonValue(std::move(array));
}

[[nodiscard]] JsonValue score_object(const OfficialScore& score) {
    JsonValue::Object object;
    object.emplace("lifetimeDistinct", JsonValue(static_cast<std::int64_t>(score.lifetimeDistinct)));
    object.emplace("totalDailyDistinct", JsonValue(static_cast<std::int64_t>(score.totalDailyDistinct)));
    object.emplace("totalServings", JsonValue(static_cast<std::int64_t>(score.totalServings)));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue day_score_object(const DayScore& score) {
    JsonValue::Object object;
    object.emplace("brandsMask", JsonValue(score.brands.wire_string()));
    object.emplace("dailyDistinct", JsonValue(static_cast<std::int64_t>(score.dailyDistinct)));
    object.emplace("servings", JsonValue(static_cast<std::int64_t>(score.servings)));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue agent_object(const AgentState& agent) {
    JsonValue::Object object;
    object.emplace("kind", JsonValue(static_cast<std::int64_t>(agent.kind)));
    object.emplace("pos", JsonValue(static_cast<std::int64_t>(agent.position)));
    object.emplace("fuel", JsonValue(static_cast<std::int64_t>(agent.fuel)));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue agents_array(const std::vector<AgentState>& agents) {
    JsonValue::Array array;
    array.reserve(agents.size());
    for (const AgentState& agent : agents) {
        array.push_back(agent_object(agent));
    }
    return JsonValue(std::move(array));
}

[[nodiscard]] JsonValue state_object(const MatchConfig& config, const DayState& state) {
    JsonValue::Object object;
    object.emplace("endsAt", JsonValue(state.endsAt));
    object.emplace("day", JsonValue(static_cast<std::int64_t>(state.dayNumber)));
    object.emplace("agents", agents_array(state.agents));
    JsonValue::Array teams;
    teams.reserve(state.others.size());
    for (const OtherTeamState& team : state.others) {
        JsonValue::Object teamObject;
        teamObject.emplace("teamId", JsonValue(static_cast<std::int64_t>(team.teamId)));
        teamObject.emplace("agents", agents_array(team.agents));
        teams.emplace_back(std::move(teamObject));
    }
    object.emplace("others", JsonValue(std::move(teams)));
    JsonValue::Array traffics;
    traffics.reserve(config.roadCells.size());
    for (const CellId road : config.roadCells) {
        JsonValue::Object traffic;
        traffic.emplace("pos", JsonValue(static_cast<std::int64_t>(road)));
        traffic.emplace(
            "status",
            JsonValue(static_cast<std::int64_t>(state.roadStatuses.at(static_cast<std::size_t>(road)))));
        traffics.emplace_back(std::move(traffic));
    }
    object.emplace("traffics", JsonValue(std::move(traffics)));
    return JsonValue(std::move(object));
}

[[nodiscard]] const char* deadline_class_name(DeadlineClass deadlineClass) {
    switch (deadlineClass) {
    case DeadlineClass::Emergency:
        return "emergency";
    case DeadlineClass::Short:
        return "short";
    case DeadlineClass::Normal:
        return "normal";
    case DeadlineClass::Long:
        return "long";
    }
    return "unknown";
}

[[nodiscard]] const char* reservation_evidence_name(ReservationEvidence evidence) {
    switch (evidence) {
    case ReservationEvidence::RelaxationOnly:
        return "relaxation-only";
    case ReservationEvidence::WitnessBacked:
        return "witness-backed";
    case ReservationEvidence::Proven:
        return "proven";
    }
    return "unknown";
}

[[nodiscard]] JsonValue deadline_object(const DeadlineProfile& deadline) {
    JsonValue::Object object;
    object.emplace("class", JsonValue(deadline_class_name(deadline.deadlineClass)));
    object.emplace("totalMs", JsonValue(static_cast<std::int64_t>(deadline.total.count())));
    object.emplace("seedMs", JsonValue(static_cast<std::int64_t>(deadline.seed.count())));
    object.emplace("fastViabilityMs", JsonValue(static_cast<std::int64_t>(deadline.fastViability.count())));
    object.emplace("searchMs", JsonValue(static_cast<std::int64_t>(deadline.search.count())));
    object.emplace("searchSoftMs", JsonValue(static_cast<std::int64_t>(deadline.searchSoft.count())));
    object.emplace("certificationMs", JsonValue(static_cast<std::int64_t>(deadline.certification.count())));
    object.emplace("networkMs", JsonValue(static_cast<std::int64_t>(deadline.network.count())));
    object.emplace("meetsMinimumFloors", JsonValue(deadline.meetsMinimumFloors));
    object.emplace("p99Calibrated", JsonValue(deadline.p99Calibrated));
    object.emplace("competitionReady", JsonValue(deadline.competitionReady));
    object.emplace("calibrationVersion", JsonValue(deadline.calibrationVersion));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue timing_object(const DecisionTiming& timing) {
    JsonValue::Object object;
    object.emplace("incumbentMs", JsonValue(static_cast<std::int64_t>(timing.incumbent.count())));
    object.emplace("fastPathMs", JsonValue(static_cast<std::int64_t>(timing.fastPath.count())));
    object.emplace(
        "columnGenerationMs",
        JsonValue(static_cast<std::int64_t>(timing.columnGeneration.count())));
    object.emplace(
        "initialMasterMs",
        JsonValue(static_cast<std::int64_t>(timing.initialMaster.count())));
    object.emplace("searchMs", JsonValue(static_cast<std::int64_t>(timing.search.count())));
    object.emplace(
        "independentGeneratorsMs",
        JsonValue(static_cast<std::int64_t>(timing.independentGenerators.count())));
    object.emplace(
        "candidatePreparationMs",
        JsonValue(static_cast<std::int64_t>(timing.candidatePreparation.count())));
    object.emplace("certificationMs", JsonValue(static_cast<std::int64_t>(timing.certification.count())));
    object.emplace("alnsMs", JsonValue(static_cast<std::int64_t>(timing.alns.count())));
    object.emplace(
        "recombinationMs",
        JsonValue(static_cast<std::int64_t>(timing.recombination.count())));
    object.emplace("totalMs", JsonValue(static_cast<std::int64_t>(timing.total.count())));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue risk_policy_object(const RiskPolicy& policy) {
    JsonValue::Object object;
    object.emplace("version", JsonValue(policy.version));
    object.emplace("confidenceBasisPoints", JsonValue(static_cast<std::int64_t>(policy.confidenceBasisPoints)));
    object.emplace("safetySlack", JsonValue(static_cast<std::int64_t>(policy.safetySlack)));
    object.emplace("resolutionBasisPoints", JsonValue(static_cast<std::int64_t>(policy.resolutionBasisPoints)));
    JsonValue::Array quantiles;
    quantiles.reserve(policy.quantileBasisPoints.size());
    for (const std::uint32_t quantile : policy.quantileBasisPoints) {
        quantiles.emplace_back(static_cast<std::int64_t>(quantile));
    }
    object.emplace("quantileBasisPoints", JsonValue(std::move(quantiles)));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue manifest_object(const ScenarioManifest& manifest) {
    JsonValue::Object object;
    object.emplace("version", JsonValue(manifest.version));
    object.emplace("generatorSeed", JsonValue(static_cast<std::int64_t>(manifest.generatorSeed)));
    object.emplace("evaluatorHash", JsonValue(manifest.evaluatorHash));
    object.emplace("riskPolicyVersion", JsonValue(manifest.riskPolicyVersion));
    object.emplace(
        "confidenceBasisPoints",
        JsonValue(static_cast<std::int64_t>(manifest.confidenceBasisPoints)));
    object.emplace("safetySlack", JsonValue(static_cast<std::int64_t>(manifest.safetySlack)));
    object.emplace(
        "resolutionBasisPoints",
        JsonValue(static_cast<std::int64_t>(manifest.resolutionBasisPoints)));
    object.emplace("quantileBasisPoints", integer_array(manifest.quantileBasisPoints));
    object.emplace("totalWeight", JsonValue(static_cast<std::int64_t>(manifest.totalWeight)));
    object.emplace("survivalSignatureEnabled", JsonValue(manifest.survivalSignatureEnabled));
    object.emplace("usedStaticFallback", JsonValue(manifest.usedStaticFallback));
    object.emplace("requiredClassesCovered", JsonValue(manifest.requiredClassesCovered));
    object.emplace("effectiveCoverageBasisPoints", JsonValue(static_cast<std::int64_t>(manifest.effectiveCoverageBasisPoints)));
    object.emplace("calibrationErrorBasisPoints", JsonValue(static_cast<std::int64_t>(manifest.calibrationErrorBasisPoints)));
    JsonValue::Array scenarios;
    scenarios.reserve(manifest.scenarios.size());
    for (const TrafficScenario& scenario : manifest.scenarios) {
        JsonValue::Object scenarioObject;
        scenarioObject.emplace("id", JsonValue(static_cast<std::int64_t>(scenario.scenarioId)));
        scenarioObject.emplace("class", JsonValue(scenario.scenarioClass));
        scenarioObject.emplace("weight", JsonValue(static_cast<std::int64_t>(scenario.weight)));
        scenarioObject.emplace("adversarial", JsonValue(scenario.adversarial));
        scenarioObject.emplace("likely", JsonValue(scenario.likely));
        scenarioObject.emplace("privateSensitivity", JsonValue(scenario.privateSensitivity));
        scenarioObject.emplace("pessimisticFallback", JsonValue(scenario.pessimisticFallback));
        scenarioObject.emplace("jointFeasible", JsonValue(scenario.jointFeasible));
        scenarioObject.emplace("construction", JsonValue(scenario.construction));
        scenarioObject.emplace("opponentCurrentFootprint", integer_array(scenario.opponentCurrentFootprint));
        scenarioObject.emplace("opponentCarryFootprint", integer_array(scenario.opponentCarryFootprint));
        scenarios.emplace_back(std::move(scenarioObject));
    }
    object.emplace("scenarios", JsonValue(std::move(scenarios)));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue viability_object(const ViabilityBounds& viability) {
    JsonValue::Object object;
    object.emplace("lowerBound", score_object(viability.lowerBound));
    object.emplace("upperBound", score_object(viability.upperBound));
    object.emplace("pessimisticUpperBound", score_object(viability.pessimisticUpperBound));
    object.emplace("coverageCap", JsonValue(static_cast<std::int64_t>(viability.coverageCap)));
    object.emplace("coverageSafe", JsonValue(static_cast<std::int64_t>(viability.coverageSafe)));
    object.emplace("confidenceCoverage", JsonValue(static_cast<std::int64_t>(viability.confidenceCoverage)));
    object.emplace("optimisticLatestDayByBrand", integer_array(viability.optimisticLatestDayByBrand));
    object.emplace("latestSafeDayByBrand", integer_array(viability.latestSafeDayByBrand));
    object.emplace("slackByBrand", integer_array(viability.slackByBrand));
    object.emplace("optimisticAgentDaySlotsByBrand", integer_array(viability.optimisticAgentDaySlotsByBrand));
    object.emplace("safeAgentDaySlotsByBrand", integer_array(viability.safeAgentDaySlotsByBrand));
    object.emplace("matchingRelaxationFeasible", JsonValue(viability.matchingRelaxationFeasible));
    object.emplace("deadlineReached", JsonValue(viability.deadlineReached));
    JsonValue::Array reservations;
    reservations.reserve(viability.reservations.size());
    for (const MandatoryReservation& reservation : viability.reservations) {
        JsonValue::Object reservationObject;
        reservationObject.emplace("brandIndex", JsonValue(static_cast<std::int64_t>(reservation.brandIndex)));
        reservationObject.emplace("representativeSpot", JsonValue(static_cast<std::int64_t>(reservation.representativeSpot)));
        reservationObject.emplace("latestSafeDay", JsonValue(static_cast<std::int64_t>(reservation.latestSafeDay)));
        reservationObject.emplace("proven", JsonValue(is_proven_reservation(reservation)));
        reservationObject.emplace("evidence", JsonValue(reservation_evidence_name(reservation.evidence)));
        reservations.emplace_back(std::move(reservationObject));
    }
    object.emplace("reservations", JsonValue(std::move(reservations)));
    JsonValue::Array conditionalTiers;
    conditionalTiers.reserve(viability.conditionalTiers.size());
    for (const ConditionalTierBounds& conditional : viability.conditionalTiers) {
        JsonValue::Object item;
        item.emplace("coverage", JsonValue(static_cast<std::int64_t>(conditional.coverage)));
        item.emplace("lowerDailyDistinct", JsonValue(static_cast<std::int64_t>(conditional.lowerDailyDistinct)));
        item.emplace("upperDailyDistinct", JsonValue(static_cast<std::int64_t>(conditional.upperDailyDistinct)));
        item.emplace("lowerServings", JsonValue(static_cast<std::int64_t>(conditional.lowerServings)));
        item.emplace("upperServings", JsonValue(static_cast<std::int64_t>(conditional.upperServings)));
        item.emplace("witnessBacked", JsonValue(conditional.witnessBacked));
        conditionalTiers.emplace_back(std::move(item));
    }
    object.emplace("conditionalTiers", JsonValue(std::move(conditionalTiers)));
    JsonValue::Array frontier;
    frontier.reserve(viability.frontier.size());
    for (const ViabilityFrontierPoint& point : viability.frontier) {
        JsonValue::Object item;
        item.emplace("coverage", JsonValue(static_cast<std::int64_t>(point.coverage)));
        item.emplace("riskRank", JsonValue(static_cast<std::int64_t>(point.riskRank)));
        item.emplace("lowerBound", score_object(point.lowerBound));
        item.emplace("upperBound", score_object(point.upperBound));
        item.emplace("scenarioId", JsonValue(static_cast<std::int64_t>(point.scenarioId)));
        item.emplace("scenarioClass", JsonValue(point.scenarioClass));
        item.emplace("witnessCertified", JsonValue(point.witness.certified));
        frontier.emplace_back(std::move(item));
    }
    object.emplace("frontier", JsonValue(std::move(frontier)));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue trace_object(const StepTrace& trace) {
    JsonValue::Object object;
    object.emplace("stepCount", JsonValue(static_cast<std::int64_t>(trace.stepCount)));
    object.emplace("agentCount", JsonValue(static_cast<std::int64_t>(trace.agentCount)));
    object.emplace("positions", integer_array(trace.positions));
    object.emplace("fuels", integer_array(trace.fuels));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue simulation_object(const SimulationResult& simulation) {
    JsonValue::Object object;
    object.emplace("valid", JsonValue(simulation.valid));
    object.emplace("score", day_score_object(simulation.score));
    object.emplace("finalAgents", agents_array(simulation.finalAgents));
    object.emplace("roadFootprint", integer_array(simulation.roadFootprint));
    JsonValue::Array claims;
    claims.reserve(simulation.claims.size());
    for (const ClaimEvent& claim : simulation.claims) {
        JsonValue::Object claimObject;
        claimObject.emplace("agent", JsonValue(static_cast<std::int64_t>(claim.agent)));
        claimObject.emplace("spot", JsonValue(static_cast<std::int64_t>(claim.spot)));
        claimObject.emplace("step", JsonValue(static_cast<std::int64_t>(claim.step)));
        claimObject.emplace("served", JsonValue(claim.served));
        claims.emplace_back(std::move(claimObject));
    }
    object.emplace("claims", JsonValue(std::move(claims)));
    object.emplace("trace", trace_object(simulation.trace));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue terminal_slack_object(const TerminalSlack& slack) {
    JsonValue::Object object;
    object.emplace("worstRemainingBrandSteps", JsonValue(static_cast<std::int64_t>(slack.worstRemainingBrandSteps)));
    object.emplace("totalRemainingBrandSteps", JsonValue(static_cast<std::int64_t>(slack.totalRemainingBrandSteps)));
    object.emplace("patrolFuelReserve", JsonValue(static_cast<std::int64_t>(slack.patrolFuelReserve)));
    object.emplace("overnightSpotCount", JsonValue(static_cast<std::int64_t>(slack.overnightSpotCount)));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue traffic_safety_object(const TrafficSafety& safety) {
    JsonValue::Object object;
    object.emplace("thresholdCrossings", JsonValue(static_cast<std::int64_t>(safety.thresholdCrossings)));
    object.emplace("thresholdBandRoads", JsonValue(static_cast<std::int64_t>(safety.thresholdBandRoads)));
    object.emplace("totalRoadStays", JsonValue(static_cast<std::int64_t>(safety.totalRoadStays)));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue profile_object(const CandidateProfile& profile) {
    JsonValue::Object object;
    object.emplace("coverageCap", JsonValue(static_cast<std::int64_t>(profile.coverageCap)));
    object.emplace("coverageSafe", JsonValue(static_cast<std::int64_t>(profile.coverageSafe)));
    object.emplace("confidenceCoverage", JsonValue(static_cast<std::int64_t>(profile.confidenceCoverage)));
    object.emplace("certifiedLowerBound", score_object(profile.certifiedLowerBound));
    object.emplace("provisionalLowerBound", score_object(profile.provisionalLowerBound));
    object.emplace("validUpperBound", score_object(profile.validUpperBound));
    object.emplace("hasValidUpperBound", JsonValue(profile.hasValidUpperBound));
    JsonValue::Array scenarioUpperBounds;
    scenarioUpperBounds.reserve(profile.scenarioValidUpperBounds.size());
    for (const OfficialScore& upperBound : profile.scenarioValidUpperBounds) {
        scenarioUpperBounds.push_back(score_object(upperBound));
    }
    object.emplace("scenarioValidUpperBounds", JsonValue(std::move(scenarioUpperBounds)));
    object.emplace("provisional", JsonValue(profile.provisional));
    object.emplace("scenarioWeights", integer_array(profile.scenarioWeights));
    object.emplace("survivalSignature", integer_array(profile.survivalSignature));
    JsonValue::Array quantiles;
    quantiles.reserve(profile.quantiles.size());
    for (const OfficialScore& quantile : profile.quantiles) {
        quantiles.push_back(score_object(quantile));
    }
    object.emplace("quantiles", JsonValue(std::move(quantiles)));
    JsonValue::Array outcomes;
    outcomes.reserve(profile.outcomes.size());
    for (const ScenarioOutcome& outcome : profile.outcomes) {
        JsonValue::Object outcomeObject;
        outcomeObject.emplace("score", score_object(outcome.score));
        outcomeObject.emplace("witnessScore", score_object(outcome.witness.score));
        outcomeObject.emplace("certified", JsonValue(outcome.witness.certified));
        outcomeObject.emplace("lowerBoundOnly", JsonValue(outcome.witness.lowerBoundOnly));
        JsonValue::Array futurePlans;
        futurePlans.reserve(outcome.witness.futurePlans.size());
        for (const DayPlan& plan : outcome.witness.futurePlans) {
            futurePlans.push_back(serialize_day_plan(plan));
        }
        outcomeObject.emplace("futurePlans", JsonValue(std::move(futurePlans)));
        outcomes.emplace_back(std::move(outcomeObject));
    }
    object.emplace("outcomes", JsonValue(std::move(outcomes)));
    JsonValue::Array conditionalTiers;
    conditionalTiers.reserve(profile.conditionalTiers.size());
    for (const ConditionalTierBounds& conditional : profile.conditionalTiers) {
        JsonValue::Object item;
        item.emplace("coverage", JsonValue(static_cast<std::int64_t>(conditional.coverage)));
        item.emplace("lowerDailyDistinct", JsonValue(static_cast<std::int64_t>(conditional.lowerDailyDistinct)));
        item.emplace("upperDailyDistinct", JsonValue(static_cast<std::int64_t>(conditional.upperDailyDistinct)));
        item.emplace("lowerServings", JsonValue(static_cast<std::int64_t>(conditional.lowerServings)));
        item.emplace("upperServings", JsonValue(static_cast<std::int64_t>(conditional.upperServings)));
        item.emplace("witnessBacked", JsonValue(conditional.witnessBacked));
        conditionalTiers.emplace_back(std::move(item));
    }
    object.emplace("conditionalTiers", JsonValue(std::move(conditionalTiers)));
    JsonValue::Array frontier;
    frontier.reserve(profile.frontier.size());
    for (const ViabilityFrontierPoint& point : profile.frontier) {
        JsonValue::Object item;
        item.emplace("coverage", JsonValue(static_cast<std::int64_t>(point.coverage)));
        item.emplace("riskRank", JsonValue(static_cast<std::int64_t>(point.riskRank)));
        item.emplace("lowerBound", score_object(point.lowerBound));
        item.emplace("upperBound", score_object(point.upperBound));
        item.emplace("scenarioId", JsonValue(static_cast<std::int64_t>(point.scenarioId)));
        item.emplace("scenarioClass", JsonValue(point.scenarioClass));
        item.emplace("witnessCertified", JsonValue(point.witness.certified));
        frontier.emplace_back(std::move(item));
    }
    object.emplace("frontier", JsonValue(std::move(frontier)));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue master_diagnostics_object(const MasterDiagnostics& diagnostics) {
    JsonValue::Object object;
    object.emplace("combinationsVisited", JsonValue(static_cast<std::int64_t>(diagnostics.combinationsVisited)));
    object.emplace(
        "beamCombinationsVisited",
        JsonValue(static_cast<std::int64_t>(diagnostics.beamCombinationsVisited)));
    object.emplace(
        "depthFirstCombinationsVisited",
        JsonValue(static_cast<std::int64_t>(diagnostics.depthFirstCombinationsVisited)));
    object.emplace(
        "branchOrderingCalls",
        JsonValue(static_cast<std::int64_t>(diagnostics.branchOrderingCalls)));
    object.emplace(
        "upperBoundChecks",
        JsonValue(static_cast<std::int64_t>(diagnostics.upperBoundChecks)));
    object.emplace(
        "upperBoundPrunes",
        JsonValue(static_cast<std::int64_t>(diagnostics.upperBoundPrunes)));
    object.emplace(
        "bundleUpperBoundChecks",
        JsonValue(static_cast<std::int64_t>(
            diagnostics.bundleUpperBoundChecks)));
    object.emplace(
        "bundleUpperBoundPrunes",
        JsonValue(static_cast<std::int64_t>(
            diagnostics.bundleUpperBoundPrunes)));
    object.emplace(
        "bundleBrandFrontierStates",
        JsonValue(static_cast<std::int64_t>(
            diagnostics.bundleBrandFrontierStates)));
    object.emplace(
        "bundleBrandFrontierFallbacks",
        JsonValue(static_cast<std::int64_t>(
            diagnostics.bundleBrandFrontierFallbacks)));
    object.emplace(
        "bundlePrunes",
        JsonValue(static_cast<std::int64_t>(diagnostics.bundlePrunes)));
    object.emplace(
        "exactBundlesDiscovered",
        JsonValue(static_cast<std::int64_t>(diagnostics.exactBundlesDiscovered)));
    object.emplace(
        "exactBundlesEvaluated",
        JsonValue(static_cast<std::int64_t>(diagnostics.exactBundlesEvaluated)));
    object.emplace(
        "exactBundlesAccepted",
        JsonValue(static_cast<std::int64_t>(diagnostics.exactBundlesAccepted)));
    object.emplace(
        "bestExactBundleScore",
        score_object(diagnostics.bestExactBundleScore));
    object.emplace(
        "partialSynchronizationChecks",
        JsonValue(static_cast<std::int64_t>(diagnostics.partialSynchronizationChecks)));
    object.emplace(
        "partialSynchronizationPrunes",
        JsonValue(static_cast<std::int64_t>(diagnostics.partialSynchronizationPrunes)));
    object.emplace("simulatorValidCombinations", JsonValue(static_cast<std::int64_t>(diagnostics.simulatorValidCombinations)));
    object.emplace("branchesPruned", JsonValue(static_cast<std::int64_t>(diagnostics.branchesPruned)));
    object.emplace("stockCapacityConflicts", JsonValue(static_cast<std::int64_t>(diagnostics.stockCapacityConflicts)));
    object.emplace("prefixConflicts", JsonValue(static_cast<std::int64_t>(diagnostics.prefixConflicts)));
    object.emplace("hotspotPromotions", JsonValue(static_cast<std::int64_t>(diagnostics.hotspotPromotions)));
    object.emplace("capCuts", JsonValue(static_cast<std::int64_t>(diagnostics.capCuts)));
    object.emplace("prefixCuts", JsonValue(static_cast<std::int64_t>(diagnostics.prefixCuts)));
    object.emplace("cutRounds", JsonValue(static_cast<std::int64_t>(diagnostics.cutRounds)));
    object.emplace("stockCreditDenials", JsonValue(static_cast<std::int64_t>(diagnostics.stockCreditDenials)));
    object.emplace("exactCreditMismatches", JsonValue(static_cast<std::int64_t>(diagnostics.exactCreditMismatches)));
    object.emplace("criticalRoadPromotions", JsonValue(static_cast<std::int64_t>(diagnostics.criticalRoadPromotions)));
    object.emplace(
        "synchronizationConflicts",
        JsonValue(static_cast<std::int64_t>(diagnostics.synchronizationConflicts)));
    object.emplace(
        "duplicatePlansSkipped",
        JsonValue(static_cast<std::int64_t>(diagnostics.duplicatePlansSkipped)));
    object.emplace(
        "invalidPlanCombinations",
        JsonValue(static_cast<std::int64_t>(diagnostics.invalidPlanCombinations)));
    object.emplace(
        "reservationConflicts",
        JsonValue(static_cast<std::int64_t>(diagnostics.reservationConflicts)));
    object.emplace(
        "roundPreparationMicroseconds",
        JsonValue(diagnostics.roundPreparationMicroseconds));
    object.emplace(
        "beamConstructionMicroseconds",
        JsonValue(diagnostics.beamConstructionMicroseconds));
    object.emplace(
        "beamEvaluationMicroseconds",
        JsonValue(diagnostics.beamEvaluationMicroseconds));
    object.emplace(
        "depthFirstSearchMicroseconds",
        JsonValue(diagnostics.depthFirstSearchMicroseconds));
    object.emplace(
        "populationMaintenanceMicroseconds",
        JsonValue(diagnostics.populationMaintenanceMicroseconds));
    object.emplace("nativeExactStockCredits", JsonValue(diagnostics.nativeExactStockCredits));
    object.emplace("stockCappedSearchOrder", JsonValue(diagnostics.stockCappedSearchOrder));
    object.emplace(
        "bundleAwareUpperBound",
        JsonValue(diagnostics.bundleAwareUpperBound));
    object.emplace("deadlineReached", JsonValue(diagnostics.deadlineReached));
    object.emplace("searchComplete", JsonValue(diagnostics.searchComplete));
    object.emplace("optimisticUpperBound", score_object(diagnostics.optimisticUpperBound));
    object.emplace(
        "searchGuidanceUpperBound",
        score_object(diagnostics.searchGuidanceUpperBound));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue alns_diagnostics_object(const AlnsDiagnostics& diagnostics) {
    JsonValue::Object object;
    object.emplace("iterations", JsonValue(static_cast<std::int64_t>(diagnostics.iterations)));
    object.emplace("accepted", JsonValue(static_cast<std::int64_t>(diagnostics.accepted)));
    object.emplace("improvements", JsonValue(static_cast<std::int64_t>(diagnostics.improvements)));
    object.emplace("synthesizedRoutes", JsonValue(static_cast<std::int64_t>(diagnostics.synthesizedRoutes)));
    object.emplace("synthesizedAccepted", JsonValue(static_cast<std::int64_t>(diagnostics.synthesizedAccepted)));
    object.emplace(
        "proofGuidedIterations",
        JsonValue(static_cast<std::int64_t>(diagnostics.proofGuidedIterations)));
    object.emplace(
        "proofGuidedRoutes",
        JsonValue(static_cast<std::int64_t>(diagnostics.proofGuidedRoutes)));
    object.emplace(
        "proofGuidedAccepted",
        JsonValue(static_cast<std::int64_t>(diagnostics.proofGuidedAccepted)));
    object.emplace(
        "proofGuidedImprovements",
        JsonValue(static_cast<std::int64_t>(diagnostics.proofGuidedImprovements)));
    object.emplace("poolRoutesConsidered", JsonValue(static_cast<std::int64_t>(diagnostics.poolRoutesConsidered)));
    object.emplace("poolNovelRoutes", JsonValue(static_cast<std::int64_t>(diagnostics.poolNovelRoutes)));
    object.emplace("poolRetainedRoutes", JsonValue(static_cast<std::int64_t>(diagnostics.poolRetainedRoutes)));
    object.emplace(
        "recombinationCandidates",
        JsonValue(static_cast<std::int64_t>(diagnostics.recombinationCandidates)));
    object.emplace(
        "recombinationImprovements",
        JsonValue(static_cast<std::int64_t>(diagnostics.recombinationImprovements)));
    object.emplace("recombinationDeadlineSkipped", JsonValue(diagnostics.recombinationDeadlineSkipped));
    object.emplace("attemptedByOperator", integer_array(diagnostics.attemptedByOperator));
    object.emplace("acceptedByOperator", integer_array(diagnostics.acceptedByOperator));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue cache_repair_object(const CacheRepairDiagnostics& diagnostics) {
    JsonValue::Object object;
    object.emplace("eligibleContingencies", JsonValue(static_cast<std::int64_t>(diagnostics.eligibleContingencies)));
    object.emplace("reusedContingencies", JsonValue(static_cast<std::int64_t>(diagnostics.reusedContingencies)));
    object.emplace("rejectedContingencies", JsonValue(static_cast<std::int64_t>(diagnostics.rejectedContingencies)));
    object.emplace("deadlineReached", JsonValue(diagnostics.deadlineReached));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue lexicographic_gap_object(const LexicographicGapDiagnostics& gap) {
    JsonValue::Object object;
    object.emplace("lowerBound", score_object(gap.lowerBound));
    object.emplace("upperBound", score_object(gap.upperBound));
    object.emplace("componentGaps", integer_array(gap.componentGaps));
    object.emplace("firstOpenTier", JsonValue(static_cast<std::int64_t>(gap.firstOpenTier)));
    object.emplace("validEnvelope", JsonValue(gap.validEnvelope));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue optimality_gap_object(const OptimalityGapDiagnostics& diagnostics) {
    JsonValue::Object object;
    object.emplace("todayPortfolio", lexicographic_gap_object(diagnostics.todayPortfolio));
    object.emplace("candidateHorizon", lexicographic_gap_object(diagnostics.candidateHorizon));
    object.emplace("viabilityHorizon", lexicographic_gap_object(diagnostics.viabilityHorizon));
    object.emplace("absoluteHorizon", lexicographic_gap_object(diagnostics.absoluteHorizon));
    object.emplace("portfolioSearchComplete", JsonValue(diagnostics.portfolioSearchComplete));
    object.emplace("viabilityDeadlineReached", JsonValue(diagnostics.viabilityDeadlineReached));
    return JsonValue(std::move(object));
}

[[nodiscard]] JsonValue audit_object(const DecisionAudit& audit) {
    JsonValue::Object object;
    object.emplace("selectionReason", JsonValue(audit.selectionReason));
    object.emplace("promotedCriticalRoads", integer_array(audit.promotedCriticalRoads));
    object.emplace("privateSensitivityRoads", integer_array(audit.privateSensitivityRoads));
    object.emplace("portfolioColumnsByAgent", integer_array(audit.portfolioColumnsByAgent));
    object.emplace("portfolioBrandCountsByAgent", integer_array(audit.portfolioBrandCountsByAgent));
    object.emplace(
        "portfolioMaximumServingsByAgent",
        integer_array(audit.portfolioMaximumServingsByAgent));
    object.emplace(
        "portfolioHarvestExtensionsByAgent",
        integer_array(audit.portfolioHarvestExtensionsByAgent));
    JsonValue::Array terminalCellsByAgent;
    terminalCellsByAgent.reserve(audit.portfolioTerminalCellsByAgent.size());
    for (const std::vector<CellId>& terminalCells :
         audit.portfolioTerminalCellsByAgent) {
        terminalCellsByAgent.push_back(integer_array(terminalCells));
    }
    object.emplace(
        "portfolioTerminalCellsByAgent",
        JsonValue(std::move(terminalCellsByAgent)));
    JsonValue::Object columnGeneration;
    columnGeneration.emplace(
        "criticalRoads",
        integer_array(audit.columnGeneration.criticalRoads));
    columnGeneration.emplace(
        "agentMilliseconds",
        integer_array(audit.columnGeneration.agentMilliseconds));
    columnGeneration.emplace(
        "agentParetoQueries",
        integer_array(audit.columnGeneration.agentParetoQueries));
    columnGeneration.emplace(
        "coordinationMilliseconds",
        JsonValue(audit.columnGeneration.coordinationMilliseconds));
    columnGeneration.emplace(
        "coordinationParetoQueries",
        JsonValue(static_cast<std::int64_t>(audit.columnGeneration.coordinationParetoQueries)));
    columnGeneration.emplace(
        "exactOrienteeringSupportedAgents",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringSupportedAgents)));
    columnGeneration.emplace(
        "exactOrienteeringCompleteAgents",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringCompleteAgents)));
    columnGeneration.emplace(
        "exactOrienteeringCacheHits",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringCacheHits)));
    columnGeneration.emplace(
        "exactOrienteeringSettledStates",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringSettledStates)));
    columnGeneration.emplace(
        "exactOrienteeringTerminalVariants",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringTerminalVariants)));
    columnGeneration.emplace(
        "exactOrienteeringBundles",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringBundles)));
    columnGeneration.emplace(
        "exactOrienteeringSeedServings",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringSeedServings)));
    columnGeneration.emplace(
        "exactOrienteeringLocalServings",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringLocalServings)));
    columnGeneration.emplace(
        "exactOrienteeringFeasibilityNodes",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringFeasibilityNodes)));
    columnGeneration.emplace(
        "exactOrienteeringOverlapFeasibilityNodes",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringOverlapFeasibilityNodes)));
    columnGeneration.emplace(
        "exactOrienteeringFeasibilityImprovements",
        JsonValue(static_cast<std::int64_t>(
            audit.columnGeneration.exactOrienteeringFeasibilityImprovements)));
    columnGeneration.emplace(
        "exactOrienteeringFeasibilityImproved",
        JsonValue(audit.columnGeneration.exactOrienteeringFeasibilityImproved));
    columnGeneration.emplace(
        "exactOrienteeringOverlapFeasibilityImproved",
        JsonValue(
            audit.columnGeneration.exactOrienteeringOverlapFeasibilityImproved));
    columnGeneration.emplace(
        "exactOrienteeringMilliseconds",
        JsonValue(audit.columnGeneration.exactOrienteeringMilliseconds));
    columnGeneration.emplace(
        "exactOrienteeringEnumerationMilliseconds",
        JsonValue(
            audit.columnGeneration
                .exactOrienteeringEnumerationMilliseconds));
    columnGeneration.emplace(
        "exactOrienteeringFinalizationMilliseconds",
        JsonValue(
            audit.columnGeneration
                .exactOrienteeringFinalizationMilliseconds));
    columnGeneration.emplace(
        "exactOrienteeringDeadlineRemainingAtStartMilliseconds",
        JsonValue(
            audit.columnGeneration
                .exactOrienteeringDeadlineRemainingAtStartMilliseconds));
    columnGeneration.emplace(
        "exactOrienteeringDeadlineOverrunMilliseconds",
        JsonValue(
            audit.columnGeneration
                .exactOrienteeringDeadlineOverrunMilliseconds));
    columnGeneration.emplace("deadlineReached", JsonValue(audit.columnGeneration.deadlineReached));
    JsonValue::Object pareto;
    pareto.emplace("queries", JsonValue(static_cast<std::int64_t>(audit.columnGeneration.pareto.queries)));
    pareto.emplace("cacheHits", JsonValue(static_cast<std::int64_t>(audit.columnGeneration.pareto.cacheHits)));
    pareto.emplace("cacheMisses", JsonValue(static_cast<std::int64_t>(audit.columnGeneration.pareto.cacheMisses)));
    pareto.emplace("cacheClears", JsonValue(static_cast<std::int64_t>(audit.columnGeneration.pareto.cacheClears)));
    pareto.emplace(
        "resourceBoundCacheHits",
        JsonValue(static_cast<std::int64_t>(audit.columnGeneration.pareto.resourceBoundCacheHits)));
    pareto.emplace(
        "resourceBoundCacheMisses",
        JsonValue(static_cast<std::int64_t>(audit.columnGeneration.pareto.resourceBoundCacheMisses)));
    pareto.emplace(
        "deadlineRejectedQueries",
        JsonValue(static_cast<std::int64_t>(audit.columnGeneration.pareto.deadlineRejectedQueries)));
    pareto.emplace(
        "deadlineInterruptedQueries",
        JsonValue(static_cast<std::int64_t>(audit.columnGeneration.pareto.deadlineInterruptedQueries)));
    pareto.emplace("queuePops", JsonValue(audit.columnGeneration.pareto.queuePops));
    pareto.emplace("labelsGenerated", JsonValue(audit.columnGeneration.pareto.labelsGenerated));
    pareto.emplace(
        "labelsDominanceRejected",
        JsonValue(audit.columnGeneration.pareto.labelsDominanceRejected));
    pareto.emplace("labelsDominated", JsonValue(audit.columnGeneration.pareto.labelsDominated));
    pareto.emplace(
        "labelsPrunedByCap",
        JsonValue(audit.columnGeneration.pareto.labelsPrunedByCap));
    pareto.emplace(
        "labelsPrunedByResourceBound",
        JsonValue(audit.columnGeneration.pareto.labelsPrunedByResourceBound));
    columnGeneration.emplace("pareto", JsonValue(std::move(pareto)));
    object.emplace("columnGeneration", JsonValue(std::move(columnGeneration)));
    object.emplace(
        "independentRoutesGenerated",
        JsonValue(static_cast<std::int64_t>(audit.independentRoutesGenerated)));
    object.emplace(
        "independentPlansEvaluated",
        JsonValue(static_cast<std::int64_t>(audit.independentPlansEvaluated)));
    object.emplace(
        "independentSearchNodes",
        JsonValue(static_cast<std::int64_t>(audit.independentSearchNodes)));
    object.emplace(
        "independentRollouts",
        JsonValue(static_cast<std::int64_t>(audit.independentRollouts)));
    object.emplace("independentCandidateAccepted", JsonValue(audit.independentCandidateAccepted));
    object.emplace("independentDeadlineReached", JsonValue(audit.independentDeadlineReached));
    object.emplace("optimalityGap", optimality_gap_object(audit.optimalityGap));
    JsonValue::Object profileFinalization;
    profileFinalization.emplace(
        "passes",
        JsonValue(static_cast<std::int64_t>(audit.profileFinalization.passes)));
    profileFinalization.emplace(
        "candidatesFinalized",
        JsonValue(static_cast<std::int64_t>(audit.profileFinalization.candidatesFinalized)));
    profileFinalization.emplace(
        "survivalTieGroups",
        JsonValue(static_cast<std::int64_t>(audit.profileFinalization.survivalTieGroups)));
    profileFinalization.emplace(
        "survivalProfilesRebuilt",
        JsonValue(static_cast<std::int64_t>(audit.profileFinalization.survivalProfilesRebuilt)));
    profileFinalization.emplace(
        "elapsedUs",
        JsonValue(static_cast<std::int64_t>(audit.profileFinalization.elapsed.count())));
    object.emplace("profileFinalization", JsonValue(std::move(profileFinalization)));
    JsonValue::Array candidates;
    candidates.reserve(audit.candidates.size());
    for (const CandidateAuditRecord& candidate : audit.candidates) {
        JsonValue::Object candidateObject;
        candidateObject.emplace("stableId", JsonValue(candidate.stableId));
        candidateObject.emplace("scoreAfterToday", score_object(candidate.scoreAfterToday));
        candidateObject.emplace("provisionalLowerBound", score_object(candidate.provisionalLowerBound));
        candidateObject.emplace("validUpperBound", score_object(candidate.validUpperBound));
        candidateObject.emplace("finalQuantile50", score_object(candidate.finalQuantile50));
        candidateObject.emplace(
            "finalCertifiedLowerBound",
            score_object(candidate.finalCertifiedLowerBound));
        candidateObject.emplace("terminalCells", integer_array(candidate.terminalCells));
        candidateObject.emplace("terminalFuel", integer_array(candidate.terminalFuel));
        candidateObject.emplace("certified", JsonValue(candidate.certified));
        candidateObject.emplace("selected", JsonValue(candidate.selected));
        candidateObject.emplace("w1Role", JsonValue(candidate.w1Role));
        candidateObject.emplace("disposition", JsonValue(candidate.disposition));
        candidates.emplace_back(std::move(candidateObject));
    }
    object.emplace("candidates", JsonValue(std::move(candidates)));
    return JsonValue(std::move(object));
}

} 

JsonValue serialize_decision_replay(
    const MatchConfig& config,
    const DayState& state,
    const MatchLedger& ledger,
    const DecisionResult& decision) {
    JsonValue::Object configObject;
    configObject.emplace("dayCount", JsonValue(static_cast<std::int64_t>(config.day_count())));
    configObject.emplace("agentCount", JsonValue(static_cast<std::int64_t>(config.agent_count())));
    configObject.emplace("brandCount", JsonValue(static_cast<std::int64_t>(config.brand_count())));
    configObject.emplace("cellCount", JsonValue(static_cast<std::int64_t>(config.map.cell_count())));

    JsonValue::Object candidateObject;
    candidateObject.emplace("stableId", JsonValue(decision.candidate.stableId));
    candidateObject.emplace("plan", serialize_day_plan(decision.candidate.plan));
    candidateObject.emplace("scoreAfterToday", score_object(decision.candidate.scoreAfterToday));
    candidateObject.emplace("terminalSlack", terminal_slack_object(decision.candidate.terminalSlack));
    candidateObject.emplace("trafficSafety", traffic_safety_object(decision.candidate.trafficSafety));
    candidateObject.emplace("creditedServings", JsonValue(static_cast<std::int64_t>(decision.candidate.creditedServings)));
    candidateObject.emplace("simulation", simulation_object(decision.candidate.simulation));

    JsonValue::Object decisionObject;
    decisionObject.emplace("dayNumber", JsonValue(static_cast<std::int64_t>(decision.dayNumber)));
    decisionObject.emplace("emergency", JsonValue(decision.emergency));
    decisionObject.emplace("reconciledAuthoritativeState", JsonValue(decision.reconciledAuthoritativeState));
    decisionObject.emplace("riskPolicy", risk_policy_object(decision.riskPolicy));
    decisionObject.emplace("deadline", deadline_object(decision.deadline));
    decisionObject.emplace("timing", timing_object(decision.timing));
    decisionObject.emplace("manifest", manifest_object(decision.manifest));
    decisionObject.emplace("viability", viability_object(decision.viability));
    decisionObject.emplace("candidate", JsonValue(std::move(candidateObject)));
    decisionObject.emplace("profile", profile_object(decision.profile));
    decisionObject.emplace("masterDiagnostics", master_diagnostics_object(decision.diagnostics));
    decisionObject.emplace("alns", alns_diagnostics_object(decision.alns));
    decisionObject.emplace("cacheRepair", cache_repair_object(decision.cacheRepair));
    decisionObject.emplace("audit", audit_object(decision.audit));

    JsonValue::Object result;
    result.emplace("schema", JsonValue("udon-shield-replay-v1"));
    result.emplace("config", JsonValue(std::move(configObject)));
    result.emplace("state", state_object(config, state));
    result.emplace("ledger", serialize_match_ledger(config, ledger));
    result.emplace("decision", JsonValue(std::move(decisionObject)));
    return JsonValue(std::move(result));
}

} 
