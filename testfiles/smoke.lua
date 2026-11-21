-- Small execution check: -v shows the resulting variable state.
local greeting = "hello"
n = 1 + 2 * 3 - 4 / 2

while n < 10 do
    n = n + 1
end

if n == 10 and greeting == "hello" then
    print(greeting, n)
end

if n > 10 then
    print("too big")
elseif n == 10 then
    print("ten")
else
    print("small")
end

for i = 1, 3 do
    print(i)
end

repeat
    n = n - 1
until n == 8

function add(a, b)
    return a + b
end

print(add(20, 22), not false, #greeting)
