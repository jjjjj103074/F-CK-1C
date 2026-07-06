// Commit message and PR title validation rules for F-CK-1C.
// Format: <type>(<scope>): <title>
// Example: chore(tooling): update commit rules

module.exports = {
    extends: ["@commitlint/config-conventional"],
    rules: {
        // Type is required and must stay intentionally small.
        "type-enum": [
            2, // error
            "always",
            ["feat", "fix", "refactor", "docs", "chore"],
        ],

        // Scope is required so commit history remains easy to scan.
        "scope-empty": [2, "never"],
        "scope-enum": [
            2, // error
            "always",
            ["core", "efm", "cockpit", "assets", "tooling", "docs"],
        ],

        // Keep type/scope lowercase; subjects may be English or Traditional Chinese.
        "type-case": [2, "always", "lower-case"],
        "scope-case": [2, "always", "lower-case"],
        "subject-case": [0],

        // Warn only, because Chinese descriptions can be dense but still readable.
        "header-max-length": [1, "always", 100],

        "body-max-line-length": [0],
        "footer-max-line-length": [0],
    },
};
