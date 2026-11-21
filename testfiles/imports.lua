-- Keep the original `require("file")` call shape executable without adding
-- a module system: define a tiny function with the same name.
function require(name)
    return name
end

require("file")
