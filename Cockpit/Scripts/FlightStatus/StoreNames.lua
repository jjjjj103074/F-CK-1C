-- 將 DCS 掛載識別碼轉成顯示名稱，未知掛載顯示識別碼摘要。
local names = {
    ["{AIM-9L}"] = "AIM-9L",
    ["{AIM-9B}"] = "AIM-9B",
    ["{AIM-9E}"] = "AIM-9E",
    ["{AIM-9J}"] = "AIM-9J",
    ["{AIM-9JULI}"] = "AIM-9JULI",
    ["{5CE2FF2A-645A-4197-B48D-8720AC69394F}"] = "AIM-9X",
    ["{6CEB49FC-DED8-4DED-B053-E1F033FF72D3}"] = "AIM-9M",
    ["{9BFD8C90-F7AE-4e90-833B-BFD0CED0E536}"] = "AIM-9P",
    ["{8D399DDA-FF81-4F14-904D-099B34FE7918}"] = "AIM-7M",
    ["{C8E06185-7CD6-4C90-959F-044679E90751}"] = "AIM-120B",
    ["{40EF17B7-F508-45de-8566-6FFECC0C1AB8}"] = "AIM-120C",
}
return function(identifier)
    if type(identifier) ~= "string" or identifier == "" then return "UNKNOWN" end
    local name = names[identifier] or identifier:match("^%{(.+)%}$") or identifier
    name = name:gsub("[%c]", " ")
    return #name > 25 and (name:sub(1, 22) .. "...") or name
end
