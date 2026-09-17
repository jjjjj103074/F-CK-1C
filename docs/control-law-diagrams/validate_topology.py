from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import re


MODEL_PATH = Path(__file__).parent / "ControlLawDiagrams" / "F16Block25" / "LongitudinalControl.mo"
CONNECT_RE = re.compile(
    r"connect\(\s*(?P<source>[\w.]+)\s*,\s*(?P<target>[\w.]+)\s*\)"
    r"(?P<annotation>\s*annotation\(\s*Line\(.*?\)\s*\))?\s*;",
    re.DOTALL,
)
EXPECTED_ARROW_RE = re.compile(
    r"arrow\s*=\s*\{\s*Arrow\.None\s*,\s*Arrow\.Filled\s*\}"
)


@dataclass(frozen=True)
class Edge:
    source: str
    target: str
    line: int
    annotation: str | None


@dataclass(frozen=True)
class NodeContract:
    model_type: str
    inputs: tuple[str, ...]
    outputs: tuple[str, ...]


NODE_CONTRACTS = {
    "autopilotCommandSum": NodeContract("T.Sum3", ("u1", "u2", "u3"), ("y",)),
    "feedbackCommandSum": NodeContract("T.Sum2", ("u1", "u2"), ("y",)),
    "u031N3": NodeContract("G.N3ScheduledGain", ("impactPressurePsf",), ("y",)),
    "hcSummer": NodeContract("T.DifferenceLeftRightToBottom", ("left", "right"), ("bottom",)),
    "hcZMultiplier": NodeContract("T.ProductRightBottomToLeft", ("right", "bottom"), ("left",)),
    "n3aZSignal": NodeContract("T.RepeatedNetUp", ("source",), ("y",)),
    "u037N3A": NodeContract("G.N3AScheduledGain", ("impactPressurePsf",), ("y",)),
    "u034AoaLimit": NodeContract("T.Limiter", ("u",), ("y",)),
    "aoaMode7Monitor": NodeContract("T.Monitor", ("u",), ("y",)),
    "u039StandbyGain": NodeContract("T.Product2", ("u1", "u2"), ("y",)),
    "n8Schedule": NodeContract(
        "G.N8Schedule",
        ("impactPressurePsf", "staticPressurePsf", "cat3Active"),
        ("y",),
    ),
    "u010PitchStructureFilter": NodeContract(
        "D.VariableLeadLag", ("u", "pole"), ("y",)
    ),
    "thresholdReductionSum": NodeContract("T.Sum3", ("u1", "u2", "u3"), ("y",)),
    "highQSum": NodeContract("T.Sum2Top", ("u1", "u2"), ("y",)),
    "highAlphaSum": NodeContract("T.Sum2Top", ("u1", "u2"), ("y",)),
    "alphaFeedbackSum": NodeContract("T.Sum3", ("u1", "u2", "u3"), ("y",)),
    "pitchRateAugmentationSum": NodeContract("T.Sum2Top", ("u1", "u2"), ("y",)),
    "pitchRateZProduct": NodeContract("T.Product2", ("u1", "u2"), ("y",)),
    "normalAccelerationSum": NodeContract("T.Sum3", ("u1", "u2", "u3"), ("y",)),
    "alphaLoadFeedbackSum": NodeContract("T.Sum2Top", ("u1", "u2"), ("y",)),
    "loadFactorYProduct": NodeContract("T.Product2", ("u1", "u2"), ("y",)),
    "u032AoaPath": NodeContract(
        "G.F2StaticStabilityGain",
        ("u", "impactPressurePsf", "staticPressurePsf"),
        ("y",),
    ),
    "elevatorIntegratorInputSum": NodeContract(
        "T.Sum2", ("u1", "u2"), ("y",)
    ),
    "finalElevatorCommandSum": NodeContract(
        "T.Sum3", ("u1", "u2", "u3"), ("y",)
    ),
    "u040ProportionalBreakout": NodeContract("D.Deadband", ("u",), ("y",)),
    "trailingMode5Selector": NodeContract(
        "T.Selector2", ("upper", "lower", "control"), ("y",)
    ),
    "trailingFlapSum": NodeContract("T.Sum3", ("u1", "u2", "u3"), ("y",)),
}

REQUIRED_EDGES = {
    ("autopilotCommandSum.y", "feedbackCommandSum.u1"),
    ("impactPressurePsf", "n3ImpactPressure.source"),
    ("n3ImpactPressure.y", "u031N3.impactPressurePsf"),
    ("u031N3.y", "hcSummer.left"),
    ("impactPressurePsf", "n3aImpactPressure.source"),
    ("n3aImpactPressure.y", "u037N3A.impactPressurePsf"),
    ("u037N3A.y", "hcZMultiplier.right"),
    ("zTransferTerminal.pin", "n3aZSignal.source"),
    ("n3aZSignal.y", "hcZMultiplier.bottom"),
    ("hcZMultiplier.left", "hcSummer.right"),
    ("hcSummer.bottom", "structureGainMultiplier.u2"),
    ("aoaMode4Selector.y", "u034AoaLimit.u"),
    ("u034AoaLimit.y", "pitchCommandSum.u3"),
    ("feedbackOutputBranch.pin", "u039StandbyGain.u1"),
    ("standbyPitchGain", "u039StandbyGain.u2"),
    ("n8Schedule.y", "u010PitchStructureFilter.pole"),
    ("aoaCommandBranch.pin", "u032AoaPath.u"),
    ("impactPressurePsf", "u032AoaPath.impactPressurePsf"),
    ("staticPressurePsf", "u032AoaPath.staticPressurePsf"),
    ("elevatorIntegratorInputSum.y", "u040ProportionalBreakout.u"),
    ("u040ProportionalBreakout.y", "finalElevatorCommandSum.u3"),
    ("u032AoaPath.y", "finalElevatorCommandSum.u1"),
    ("u012ElevatorLimiter.y", "finalElevatorCommandSum.u2"),
    ("u022PitchAoa.y", "leadingLowerSum.u2"),
    ("highQSum.y", "highAlphaSum.u1"),
    ("alphaFeedbackSum.y", "highAlphaSum.u2"),
    ("highAlphaSum.y", "highAlphaPositive.u"),
    ("normalAccelerationSum.y", "alphaLoadFeedbackSum.u1"),
    ("alphaFeedbackPositive.y", "alphaLoadFeedbackSum.u2"),
    ("alphaLoadFeedbackSum.y", "loadFactorYProduct.u1"),
    ("trailingZero.y", "trailingMode5Selector.upper"),
    ("trailingEdgeM10", "trailingMode5Selector.lower"),
    ("trailingMode5Selector.y", "trailingFlapSum.u1"),
    ("trailingBias1_5.y", "trailingFlapSum.u2"),
}

FORBIDDEN_EDGES = {
    ("hcSummer.y", "hcZMultiplier.u1"),
    ("hcZMultiplier.y", "u037N3A.u"),
    ("u037N3A.y", "hcOutput"),
    ("firstFeedbackCommandSum.y", "secondFeedbackCommandSum.u1"),
    ("u043LowerBranch.pin", "highAlphaSum.u2"),
    ("u043LowerBranch.pin", "alphaLoadCommandSum.u2"),
    ("alphaLoadFeedbackSum.y", "alphaLoadCommandSum.u1"),
    ("alphaLoadCommandSum.y", "loadFactorYProduct.u1"),
    ("finalElevatorCommandSum.y", "u040ProportionalBreakout.u"),
    ("u040ProportionalBreakout.y", "elevatorIntegratorInputSum.u3"),
    ("leadingBias2.y", "leadingLowerSum.u2"),
    ("aoaFeedbackBranch.pin", "highAlphaSum.u2"),
    ("trailingZero.y", "trailingFlapSum.u1"),
    ("trailingEdgeM10", "trailingFlapSum.u2"),
    ("alphaFeedbackSum.y", "u032InputSum.u2"),
}

FORBIDDEN_SYMBOLS = (
    "alphaLoadCommandSum",
    "highAlphaFeedbackSum",
    "u043CommandCrossfeed",
    "u043LowerBranch",
    "u043OutputBranch",
)

EXECUTION_BLOCKERS = (
    "T.UnknownBlock",
    "T.UnknownSource",
    "T.UnresolvedGain",
    "T.LimiterWithUnknownBounds",
)

REQUIRED_LOGIC = (
    'T.Sum3 pitchCommandSum(k3=-1.0,label="COMMAND SUM")',
    'T.Sum3 autopilotCommandSum(k1=-1.0,k2=-1.0,k3=1.0,label="PA SUM")',
    'T.Sum2 transferZSum(k1=-1.0,label="Z = 1 - Y")',
    'T.Sum3 thresholdReductionSum(k2=-1.0,k3=-1.0,label="F - 13.4',
    'T.Sum2 elevatorIntegratorInputSum(label="P+I INPUT',
    'T.Sum3 finalElevatorCommandSum(label="ELEVATOR SUM")',
    'T.Sum3 leadingUpperSum(k3=-1.0,label="LEF UPPER',
)


def parse_edges(text: str) -> list[Edge]:
    edges = []
    for match in CONNECT_RE.finditer(text):
        edges.append(
            Edge(
                source=match.group("source"),
                target=match.group("target"),
                line=text.count("\n", 0, match.start()) + 1,
                annotation=match.group("annotation"),
            )
        )
    return edges


def validate_duplicate_edges(edges: list[Edge]) -> list[str]:
    counts = Counter((edge.source, edge.target) for edge in edges)
    return [
        f"duplicate edge {source} -> {target} appears {count} times"
        for (source, target), count in counts.items()
        if count > 1
    ]


def validate_input_sources(edges: list[Edge]) -> list[str]:
    sources_by_target: dict[str, list[str]] = {}
    for edge in edges:
        sources_by_target.setdefault(edge.target, []).append(edge.source)
    return [
        f"input {target} has multiple sources: {', '.join(sources)}"
        for target, sources in sources_by_target.items()
        if len(sources) > 1
    ]


def validate_arrows(edges: list[Edge]) -> list[str]:
    errors = []
    for edge in edges:
        if edge.annotation is None:
            continue
        filled_count = edge.annotation.count("Arrow.Filled")
        if EXPECTED_ARROW_RE.search(edge.annotation) is None or filled_count != 1:
            errors.append(
                f"line {edge.line}: {edge.source} -> {edge.target} must have one forward arrow"
            )
    return errors


def validate_node_contracts(text: str, edges: list[Edge]) -> list[str]:
    source_counts = Counter(edge.source for edge in edges)
    target_counts = Counter(edge.target for edge in edges)
    errors = []
    for instance, contract in NODE_CONTRACTS.items():
        declaration = rf"\b{re.escape(contract.model_type)}\s+{re.escape(instance)}\b"
        if len(re.findall(declaration, text)) != 1:
            errors.append(f"{instance} must be declared exactly once as {contract.model_type}")
        for port in contract.inputs:
            endpoint = f"{instance}.{port}"
            if target_counts[endpoint] != 1:
                errors.append(f"input {endpoint} must have exactly one source")
        for port in contract.outputs:
            endpoint = f"{instance}.{port}"
            if source_counts[endpoint] < 1:
                errors.append(f"output {endpoint} must drive at least one destination")
    return errors


def validate_known_paths(edges: list[Edge]) -> list[str]:
    actual = {(edge.source, edge.target) for edge in edges}
    errors = [
        f"required edge is missing: {source} -> {target}"
        for source, target in sorted(REQUIRED_EDGES - actual)
    ]
    errors.extend(
        f"obsolete edge is still present: {source} -> {target}"
        for source, target in sorted(FORBIDDEN_EDGES & actual)
    )
    return errors


def validate_forbidden_symbols(text: str) -> list[str]:
    return [
        f"obsolete transcription symbol is still present: {symbol}"
        for symbol in FORBIDDEN_SYMBOLS
        if re.search(rf"\b{re.escape(symbol)}\b", text)
    ]


def validate_execution_blockers(text: str) -> list[str]:
    return [
        f"non-executable transcription block is still instantiated: {symbol}"
        for symbol in EXECUTION_BLOCKERS
        if symbol in text
    ]


def validate_required_logic(text: str) -> list[str]:
    normalized_text = re.sub(r"\s+", "", text)
    return [
        f"required signed-control-law declaration is missing: {fragment}"
        for fragment in REQUIRED_LOGIC
        if re.sub(r"\s+", "", fragment) not in normalized_text
    ]


def main() -> int:
    text = MODEL_PATH.read_text(encoding="utf-8")
    edges = parse_edges(text)
    errors = []
    errors.extend(validate_duplicate_edges(edges))
    errors.extend(validate_input_sources(edges))
    errors.extend(validate_arrows(edges))
    errors.extend(validate_node_contracts(text, edges))
    errors.extend(validate_known_paths(edges))
    errors.extend(validate_forbidden_symbols(text))
    errors.extend(validate_execution_blockers(text))
    errors.extend(validate_required_logic(text))
    if errors:
        print("Topology contract failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print(f"Topology contract passed: {len(edges)} directed connections checked.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
