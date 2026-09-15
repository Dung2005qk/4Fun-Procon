// Fixture/traffic export only; never invoke the historical solver entry point.
#define main historical_solver_not_called_332
#include "../../old/harness/historical_tournament.cpp"
#undef main

int main() {
    using J = udon::JsonValue;
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            const auto q = J::parse(line);
            const auto seed = static_cast<std::uint64_t>(q.at("seed").integer());
            const int side = static_cast<int>(q.at("side").integer());
            const int days = static_cast<int>(q.at("days").integer());
            auto f = side == 8 ? generated_fixture(seed) : generated_btc_large_fixture(seed, 32, days, 8, 24);
            const int steps = f.daySteps.front();
            f.daySteps.resize(days);
            for (int d = 0; d < days; ++d) f.daySteps[d] = steps + d % 3;
            f.players = static_cast<int>(q.at("players").integer());
            if (q.at("fuel").string() == "low") f.fuelLimit = std::max(1, steps / 2);
            if (q.at("fuel").string() == "high") f.fuelLimit = 3 * steps;
            if (q.at("roadless").boolean())
                for (auto& cell : f.terrain)
                    if (cell == static_cast<int>(udon::Terrain::Road)) cell = static_cast<int>(udon::Terrain::Plain);
            auto object = J::parse(config_document(f)).object();
            object["daySeconds"] = J(J::Array(days, J(q.at("window_ms").integer() / 1000)));
            auto setup = J(std::move(object));
            const auto c = udon::parse_match_config(setup);
            J::Array external;
            for (const auto& day : opponent_footprints(f, c)) {
                J::Array cells;
                for (const auto n : day) cells.emplace_back(static_cast<std::int64_t>(n));
                external.emplace_back(std::move(cells));
            }
            std::cout << J(J::Object{{"ok", J(true)}, {"family", J(f.family)},
                {"setup", setup}, {"external_road_footprints", J(std::move(external))}}).dump() << std::endl;
        } catch (const std::exception& e) {
            std::cout << J(J::Object{{"ok", J(false)}, {"error", J(e.what())}}).dump() << std::endl;
        }
    }
}
