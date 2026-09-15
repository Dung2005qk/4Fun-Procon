"""Mechanical source patch emitter and read-only integration verification."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
STAGE = ROOT / 'artifacts/research/370/source'
FROZEN = ROOT / 'artifacts/research/357/completed/source'
HOST = ROOT / 'artifacts/research/360/completed/btc_main.cpp'
EXPECTED = {
    'src/slack_refiner.cpp': '76A34E707AFD2466919E7911D5B554581F621AD407FEDA720DB061979CA601F2',
    'include/udon/slack_refiner.hpp': '633E394F02710050AD88BAAD5F80EADCA46B2569998FFE6D0C36347538FDED3D',
}


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest().upper()


def text(p):
    return p.read_text(encoding='utf-8')


def intended():
    assert sha(HOST) == '4031F7D3A73E6D4D0ADF4182B332CAC9C494726E05B9500F7C77B27EF4BDC28A'
    result = {}
    for name, expected in EXPECTED.items():
        assert sha(FROZEN / name) == expected, name
        result[name] = text(FROZEN / name)
    raw = text(ROOT / 'src/btc_main.cpp')
    frozen = text(HOST)
    start = raw.index('[[nodiscard]] bool transient_http_status')
    end = raw.index('\n#else\n\nvoid run_http(const RuntimeOptions&)', start)
    tail = raw[raw.index('\n#endif', end) + len('\n#endif'):]
    assert frozen.endswith(tail)
    policy = frozen[frozen.index('[[nodiscard]] bool transient_http_status'):-len(tail)]
    decl = ('                udon::ResourceMarginalResult publicResourceMarginal;\n'
            '                bool resourceMarginalControl360 = false;\n')
    begin = policy.index('                    // Contract360:')
    finish = policy.index('                    if (publicContinuation.improved)', begin)
    call = policy[begin:finish]
    begin = policy.index('                if (resourceMarginalControl360) {')
    finish = policy.index('                telemetry.emplace(', begin)
    telemetry = policy[begin:finish]
    assert policy.count(decl) == 1
    restored = policy.replace(decl, '', 1).replace(call, '', 1).replace(telemetry, '', 1)
    assert restored == raw[start:end], 'whole original native HTTP policy must be preserved'
    reader = ('                        const char* resourceFlag360 = std::getenv("UDON_RESOURCE_MARGINAL_357");\n'
              '                        resourceMarginalControl360 = resourceFlag360 != nullptr &&\n'
              '                            std::string_view(resourceFlag360) == "1";\n')
    assert policy.count(reader) == 1
    integrated = policy.replace(reader, '                        resourceMarginalControl360 = true;\n', 1)
    integrated = integrated.replace('resourceMarginalControl360', 'publicResourceMarginalAuthorized')
    integrated = integrated.replace(
        '                    // Contract360: observe the treatment only after the entire\n'
        '                    // canonical public prefix; inactive runtime never reads it.\n',
        '                    // Run the certified resource suffix only after the complete\n'
        '                    // canonical public prefix and its authoritative window guard.\n')
    assert 'UDON_RESOURCE_MARGINAL_357' not in integrated
    assert integrated.count('refine_resource_marginal(') == 1
    result['src/btc_main.cpp'] = raw[:start] + integrated + raw[end:]
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--patch', action='store_true')
    args = parser.parse_args()
    wanted = intended()
    if args.patch:
        patch = ['*** Begin Patch']
        for name, new in wanted.items():
            old = text(STAGE / name)
            assert old == text(ROOT / name), 'staging source already edited'
            patch.append('*** Update File: ' + (STAGE / name).as_posix())
            diff = list(difflib.unified_diff(old.splitlines(), new.splitlines(), n=3))
            patch += ['@@' if line.startswith('@@') else line for line in diff[2:]]
        patch.append('*** End Patch')
        print(json.dumps({'patch': '\n'.join(patch)}))
        return
    for name, new in wanted.items():
        assert text(STAGE / name) == new, name
    changed = []
    hashes = {}
    for p in sorted(STAGE.rglob('*')):
        if not p.is_file():
            continue
        name = p.relative_to(STAGE).as_posix()
        if p.suffix in ('.cpp', '.hpp', '.inc') or p.name == 'CMakeLists.txt':
            hashes[name] = sha(p)
            if text(p) != text(ROOT / name):
                changed.append(name)
    assert sorted(changed) == sorted(wanted), changed
    for name in ('src/orienteering.cpp', 'src/decision.cpp', 'src/runtime.cpp', 'src/graph.cpp'):
        assert text(STAGE / name) == text(FROZEN / name), name
    report = {
        'experiment': 'INTEGRATION-RESOURCE-MARGINAL-370',
        'parent': 'c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c',
        'changes': changed, 'source_hashes': hashes,
        'complete_parent_http_policy_preserved': True,
        'frozen_resource_library_identical': True,
        'frozen360_policy_equivalent_except_explicit_research_toggle_removal': True,
        'no_source_mechanism_tuning': True, 'no_score_gate_relabeling': True,
        'script_sha256': sha(Path(__file__)),
        'installed_parent_sha256': sha(ROOT / 'build-release/udonshield_btc.exe'),
    }
    assert report['installed_parent_sha256'] == 'F97F168FE76FEF1B6226D2C2CFDAF954F116D39F427B6F157594151C461E1275'
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
