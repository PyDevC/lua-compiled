function funccall() end

function call(num)
    num = num + 1
    return num
end

function call2(num1, num2)
    local num = num1 + num2
    return num
end

call(10)
call(10 + 10)
call2(10, 20)

local a = call(10)
a = call(10) + call2(10)
