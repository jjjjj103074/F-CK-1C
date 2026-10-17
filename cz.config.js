const { definePrompt } = require("czg");

module.exports = definePrompt({
    types: [
        { value: "feat", name: "feat:     New feature or capability" },
        { value: "fix", name: "fix:      Bug fix" },
        { value: "refactor", name: "refactor: Code restructure without behavior change" },
        { value: "docs", name: "docs:     Documentation only" },
        { value: "chore", name: "chore:    Maintenance, tooling, assets, or config" },
    ],
    scopes: [
        { value: "core", name: "core:     entry, aircraft definition, SFM/FM config, weapons, runtime Lua" },
        { value: "efm", name: "efm:      C++ EFM project, DLL, MSBuild" },
        { value: "cockpit", name: "cockpit:  cockpit scripts, devices, indicators, input bindings" },
        { value: "assets", name: "assets:   Shapes, Textures, Sounds, Liveries" },
        { value: "tooling", name: "tooling:  Git hooks, commitlint, GitHub Actions, repo tools" },
        { value: "docs", name: "docs:     README, CONTRIBUTING, docs" },
    ],
    typesSearchValue: true,
    scopesSearchValue: true,
    allowCustomScopes: false,
    allowEmptyScopes: false,
    maxHeaderLength: 100,
    messages: {
        type: "Select commit type:",
        scope: "Select commit scope:",
        subject: "Write a short description:",
        body: "Longer description (optional). Use | to break lines:",
        footer: "Affected issues (optional), for example #31:",
        confirmCommit: "Confirm commit?",
    },
});
