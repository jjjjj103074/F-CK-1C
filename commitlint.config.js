// commitlint.config.js
// Commit message validation rules for F-CK-1C project.
// Format: <type>(<scope>): <title>
// Example: fix(CMS): 修正 chaff 連發模式在低速時不觸發的問題

module.exports = {
    extends: ["@commitlint/config-conventional"],
    rules: {
        // --- Type: required, must be one of the following ---
        "type-enum": [
            2, // error
            "always",
            ["feat", "fix", "refactor", "docs", "chore", "wip", "ci"],
        ],

        // --- Scope: optional, suggested list (warning if outside list) ---
        "scope-enum": [
            1, // warning only — free input is allowed
            "always",
            ["SFM", "EFM", "Cockpit_device", "Cockpit_indicators", "entry", "ci"],
        ],

        // --- Case rules: disabled to allow Chinese titles and mixed-case scopes ---
        "scope-case": [0],
        "subject-case": [0],

        // --- Title length: warning at 100 chars (Chinese-friendly) ---
        "header-max-length": [1, "always", 100],

        // --- Body/footer: no restrictions ---
        "body-max-line-length": [0],
        "footer-max-line-length": [0],
    },
};
