-- Large mixed workload fixture.
-- Generated deterministically; valid for lua-compiled and for plain Lua 5.x.
-- Exercises arithmetic, calls, recursion, branches, loops, strings and booleans.

function mix(a, b, c)
    return (a + b) * c - (a - c) / 2.0 + b * b
end

function fib(n)
    if n <= 1 then
        return n
    end
    return fib(n - 1) + fib(n - 2)
end

function fact(n)
    local out = 1
    while n > 1 do
        out = out * n
        n = n - 1
    end
    return out
end

function divisible(n, d)
    local t = n
    while t >= d do
        t = t - d
    end
    return t == 0
end

function gcd(a, b)
    while b > 0 do
        if a > b then
            a = a - b
        else
            b = b - a
        end
    end
    return a
end

function is_prime(n)
    if n < 2 then
        return false
    end
    local d = 2
    while d * d <= n do
        if divisible(n, d) then
            return false
        end
        d = d + 1
    end
    return true
end

function abs_value(n)
    if n < 0 then
        return 0 - n
    end
    return n
end

function clamp_value(x, lo, hi)
    if x < lo then
        return lo + 0
    elseif x > hi then
        return hi + 0
    end
    return x + 0
end

function sum_to(n)
    local total = 0
    for i = 1, n do
        total = total + i
    end
    return total
end

function pick(c, a, b)
    local x = a + 0
    local y = b + 0
    if c then
        return x
    else
        return y
    end
end

function same_string(a, b)
    if a == b then
        return true
    end
    return false
end

function helper_00(x)
    return x + 0.0
end

function helper_01(x)
    return x + 1.0
end

function helper_02(x)
    return x + 2.0
end

function helper_03(x)
    return x + 3.0
end

function helper_04(x)
    return x + 4.0
end

function helper_05(x)
    return x + 5.0
end

function helper_06(x)
    return x + 6.0
end

function helper_07(x)
    return x + 7.0
end

function helper_08(x)
    return x + 8.0
end

function helper_09(x)
    return x + 9.0
end

function helper_10(x)
    return x + 10.0
end

function helper_11(x)
    return x + 11.0
end

function helper_12(x)
    return x + 12.0
end

function helper_13(x)
    return x + 13.0
end

function helper_14(x)
    return x + 14.0
end

function helper_15(x)
    return x + 15.0
end

function helper_16(x)
    return x + 16.0
end

function helper_17(x)
    return x + 17.0
end

function helper_18(x)
    return x + 18.0
end

function helper_19(x)
    return x + 19.0
end

function helper_20(x)
    return x + 20.0
end

function helper_21(x)
    return x + 21.0
end

function helper_22(x)
    return x + 22.0
end

function helper_23(x)
    return x + 23.0
end

function helper_24(x)
    return x + 24.0
end

function helper_25(x)
    return x + 25.0
end

function helper_26(x)
    return x + 26.0
end

function helper_27(x)
    return x + 27.0
end

function helper_28(x)
    return x + 28.0
end

function helper_29(x)
    return x + 29.0
end

function helper_30(x)
    return x + 30.0
end

function helper_31(x)
    return x + 31.0
end

function helper_32(x)
    return x + 32.0
end

function helper_33(x)
    return x + 33.0
end

function helper_34(x)
    return x + 34.0
end

function helper_35(x)
    return x + 35.0
end

function helper_36(x)
    return x + 36.0
end

function helper_37(x)
    return x + 37.0
end

function helper_38(x)
    return x + 38.0
end

function helper_39(x)
    return x + 39.0
end

function helper_40(x)
    return x + 40.0
end

function helper_41(x)
    return x + 41.0
end

function helper_42(x)
    return x + 42.0
end

function helper_43(x)
    return x + 43.0
end

function helper_44(x)
    return x + 44.0
end

function helper_45(x)
    return x + 45.0
end

function helper_46(x)
    return x + 46.0
end

function helper_47(x)
    return x + 47.0
end

function helper_48(x)
    return x + 48.0
end

function helper_49(x)
    return x + 49.0
end

function helper_50(x)
    return x + 50.0
end

function helper_51(x)
    return x + 51.0
end

function helper_52(x)
    return x + 52.0
end

function helper_53(x)
    return x + 53.0
end

function helper_54(x)
    return x + 54.0
end

function helper_55(x)
    return x + 55.0
end

function helper_56(x)
    return x + 56.0
end

function helper_57(x)
    return x + 57.0
end

function helper_58(x)
    return x + 58.0
end

function helper_59(x)
    return x + 59.0
end

function helper_60(x)
    return x + 60.0
end

function helper_61(x)
    return x + 61.0
end

function helper_62(x)
    return x + 62.0
end

function helper_63(x)
    return x + 63.0
end

s = "a-compiled-string-value"
t = "different-string-value"
acc = 0
hits = 0
flag = true
flag2 = false
local k = 3.0

-- Long straight-line arithmetic block.
acc = acc + ((1.0 - k) * (2.0 + 3.0) / 4.0)
acc = acc - (2.0 * k + 3.0 - 4.0 / 3.0)
acc = acc + ((3.0 + 4.0) / k + 5.0 * 2.0)
acc = acc + (4.0 + 5.0 * k - 6.0 / 2.0)
acc = acc + ((5.0 - k) * (6.0 + 7.0) / 8.0)
acc = acc - (6.0 * k + 7.0 - 8.0 / 3.0)
acc = acc + ((7.0 + 8.0) / k + 9.0 * 2.0)
acc = acc + (8.0 + 9.0 * k - 10.0 / 2.0)
acc = acc + ((9.0 - k) * (10.0 + 11.0) / 12.0)
acc = acc - (10.0 * k + 11.0 - 12.0 / 3.0)
acc = acc + ((11.0 + 12.0) / k + 13.0 * 2.0)
acc = acc + (12.0 + 13.0 * k - 14.0 / 2.0)
acc = acc + ((13.0 - k) * (14.0 + 15.0) / 16.0)
acc = acc - (14.0 * k + 15.0 - 16.0 / 3.0)
acc = acc + ((15.0 + 16.0) / k + 17.0 * 2.0)
acc = acc + (16.0 + 17.0 * k - 18.0 / 2.0)
acc = acc + ((17.0 - k) * (18.0 + 19.0) / 20.0)
acc = acc - (18.0 * k + 19.0 - 20.0 / 3.0)
acc = acc + ((19.0 + 20.0) / k + 21.0 * 2.0)
acc = acc + (20.0 + 21.0 * k - 22.0 / 2.0)
acc = acc + ((21.0 - k) * (22.0 + 23.0) / 24.0)
acc = acc - (22.0 * k + 23.0 - 24.0 / 3.0)
acc = acc + ((23.0 + 24.0) / k + 25.0 * 2.0)
acc = acc + (24.0 + 25.0 * k - 26.0 / 2.0)
acc = acc + ((25.0 - k) * (26.0 + 27.0) / 28.0)
acc = acc - (26.0 * k + 27.0 - 28.0 / 3.0)
acc = acc + ((27.0 + 28.0) / k + 29.0 * 2.0)
acc = acc + (28.0 + 29.0 * k - 30.0 / 2.0)
acc = acc + ((29.0 - k) * (30.0 + 31.0) / 32.0)
acc = acc - (30.0 * k + 31.0 - 32.0 / 3.0)
acc = acc + ((31.0 + 32.0) / k + 33.0 * 2.0)
acc = acc + (32.0 + 33.0 * k - 34.0 / 2.0)
acc = acc + ((33.0 - k) * (34.0 + 35.0) / 36.0)
acc = acc - (34.0 * k + 35.0 - 36.0 / 3.0)
acc = acc + ((35.0 + 36.0) / k + 37.0 * 2.0)
acc = acc + (36.0 + 37.0 * k - 38.0 / 2.0)
acc = acc + ((37.0 - k) * (38.0 + 39.0) / 40.0)
acc = acc - (38.0 * k + 39.0 - 40.0 / 3.0)
acc = acc + ((39.0 + 40.0) / k + 41.0 * 2.0)
acc = acc + (40.0 + 41.0 * k - 42.0 / 2.0)
acc = acc + ((41.0 - k) * (42.0 + 43.0) / 44.0)
acc = acc - (42.0 * k + 43.0 - 44.0 / 3.0)
acc = acc + ((43.0 + 44.0) / k + 45.0 * 2.0)
acc = acc + (44.0 + 45.0 * k - 46.0 / 2.0)
acc = acc + ((45.0 - k) * (46.0 + 47.0) / 48.0)
acc = acc - (46.0 * k + 47.0 - 48.0 / 3.0)
acc = acc + ((47.0 + 48.0) / k + 49.0 * 2.0)
acc = acc + (48.0 + 49.0 * k - 50.0 / 2.0)
acc = acc + ((49.0 - k) * (50.0 + 51.0) / 52.0)
acc = acc - (50.0 * k + 51.0 - 52.0 / 3.0)
acc = acc + ((51.0 + 52.0) / k + 53.0 * 2.0)
acc = acc + (52.0 + 53.0 * k - 54.0 / 2.0)
acc = acc + ((53.0 - k) * (54.0 + 55.0) / 56.0)
acc = acc - (54.0 * k + 55.0 - 56.0 / 3.0)
acc = acc + ((55.0 + 56.0) / k + 57.0 * 2.0)
acc = acc + (56.0 + 57.0 * k - 58.0 / 2.0)
acc = acc + ((57.0 - k) * (58.0 + 59.0) / 60.0)
acc = acc - (58.0 * k + 59.0 - 60.0 / 3.0)
acc = acc + ((59.0 + 60.0) / k + 61.0 * 2.0)
acc = acc + (60.0 + 61.0 * k - 62.0 / 2.0)
acc = acc + ((61.0 - k) * (62.0 + 63.0) / 64.0)
acc = acc - (62.0 * k + 63.0 - 64.0 / 3.0)
acc = acc + ((63.0 + 64.0) / k + 65.0 * 2.0)
acc = acc + (64.0 + 65.0 * k - 66.0 / 2.0)
acc = acc + ((65.0 - k) * (66.0 + 67.0) / 68.0)
acc = acc - (66.0 * k + 67.0 - 68.0 / 3.0)
acc = acc + ((67.0 + 68.0) / k + 69.0 * 2.0)
acc = acc + (68.0 + 69.0 * k - 70.0 / 2.0)
acc = acc + ((69.0 - k) * (70.0 + 71.0) / 72.0)
acc = acc - (70.0 * k + 71.0 - 72.0 / 3.0)
acc = acc + ((71.0 + 72.0) / k + 73.0 * 2.0)
acc = acc + (72.0 + 73.0 * k - 74.0 / 2.0)
acc = acc + ((73.0 - k) * (74.0 + 75.0) / 76.0)
acc = acc - (74.0 * k + 75.0 - 76.0 / 3.0)
acc = acc + ((75.0 + 76.0) / k + 77.0 * 2.0)
acc = acc + (76.0 + 77.0 * k - 78.0 / 2.0)
acc = acc + ((77.0 - k) * (78.0 + 79.0) / 80.0)
acc = acc - (78.0 * k + 79.0 - 80.0 / 3.0)
acc = acc + ((79.0 + 80.0) / k + 81.0 * 2.0)
acc = acc + (80.0 + 81.0 * k - 82.0 / 2.0)
acc = acc + ((81.0 - k) * (82.0 + 83.0) / 84.0)
acc = acc - (82.0 * k + 83.0 - 84.0 / 3.0)
acc = acc + ((83.0 + 84.0) / k + 85.0 * 2.0)
acc = acc + (84.0 + 85.0 * k - 86.0 / 2.0)
acc = acc + ((85.0 - k) * (86.0 + 87.0) / 88.0)
acc = acc - (86.0 * k + 87.0 - 88.0 / 3.0)
acc = acc + ((87.0 + 88.0) / k + 89.0 * 2.0)
acc = acc + (88.0 + 89.0 * k - 90.0 / 2.0)
acc = acc + ((89.0 - k) * (90.0 + 91.0) / 92.0)
acc = acc - (90.0 * k + 91.0 - 92.0 / 3.0)
acc = acc + ((91.0 + 92.0) / k + 93.0 * 2.0)
acc = acc + (92.0 + 93.0 * k - 94.0 / 2.0)
acc = acc + ((93.0 - k) * (94.0 + 95.0) / 96.0)
acc = acc - (94.0 * k + 95.0 - 96.0 / 3.0)
acc = acc + ((95.0 + 96.0) / k + 97.0 * 2.0)
acc = acc + (96.0 + 97.0 * k - 98.0 / 2.0)
acc = acc + ((97.0 - k) * (98.0 + 99.0) / 100.0)
acc = acc - (98.0 * k + 99.0 - 100.0 / 3.0)
acc = acc + ((99.0 + 100.0) / k + 101.0 * 2.0)
acc = acc + (100.0 + 101.0 * k - 102.0 / 2.0)
acc = acc + ((101.0 - k) * (102.0 + 103.0) / 104.0)
acc = acc - (102.0 * k + 103.0 - 104.0 / 3.0)
acc = acc + ((103.0 + 104.0) / k + 105.0 * 2.0)
acc = acc + (104.0 + 105.0 * k - 106.0 / 2.0)
acc = acc + ((105.0 - k) * (106.0 + 107.0) / 108.0)
acc = acc - (106.0 * k + 107.0 - 108.0 / 3.0)
acc = acc + ((107.0 + 108.0) / k + 109.0 * 2.0)
acc = acc + (108.0 + 109.0 * k - 110.0 / 2.0)
acc = acc + ((109.0 - k) * (110.0 + 111.0) / 112.0)
acc = acc - (110.0 * k + 111.0 - 112.0 / 3.0)
acc = acc + ((111.0 + 112.0) / k + 113.0 * 2.0)
acc = acc + (112.0 + 113.0 * k - 114.0 / 2.0)
acc = acc + ((113.0 - k) * (114.0 + 115.0) / 116.0)
acc = acc - (114.0 * k + 115.0 - 116.0 / 3.0)
acc = acc + ((115.0 + 116.0) / k + 117.0 * 2.0)
acc = acc + (116.0 + 117.0 * k - 118.0 / 2.0)
acc = acc + ((117.0 - k) * (118.0 + 119.0) / 120.0)
acc = acc - (118.0 * k + 119.0 - 120.0 / 3.0)
acc = acc + ((119.0 + 120.0) / k + 121.0 * 2.0)
acc = acc + (120.0 + 121.0 * k - 122.0 / 2.0)
acc = acc + ((121.0 - k) * (122.0 + 123.0) / 124.0)
acc = acc - (122.0 * k + 123.0 - 124.0 / 3.0)
acc = acc + ((123.0 + 124.0) / k + 125.0 * 2.0)
acc = acc + (124.0 + 125.0 * k - 126.0 / 2.0)
acc = acc + ((125.0 - k) * (126.0 + 127.0) / 128.0)
acc = acc - (126.0 * k + 127.0 - 128.0 / 3.0)
acc = acc + ((127.0 + 128.0) / k + 129.0 * 2.0)
acc = acc + (128.0 + 129.0 * k - 130.0 / 2.0)
acc = acc + ((129.0 - k) * (130.0 + 131.0) / 132.0)
acc = acc - (130.0 * k + 131.0 - 132.0 / 3.0)
acc = acc + ((131.0 + 132.0) / k + 133.0 * 2.0)
acc = acc + (132.0 + 133.0 * k - 134.0 / 2.0)
acc = acc + ((133.0 - k) * (134.0 + 135.0) / 136.0)
acc = acc - (134.0 * k + 135.0 - 136.0 / 3.0)
acc = acc + ((135.0 + 136.0) / k + 137.0 * 2.0)
acc = acc + (136.0 + 137.0 * k - 138.0 / 2.0)
acc = acc + ((137.0 - k) * (138.0 + 139.0) / 140.0)
acc = acc - (138.0 * k + 139.0 - 140.0 / 3.0)
acc = acc + ((139.0 + 140.0) / k + 141.0 * 2.0)
acc = acc + (140.0 + 141.0 * k - 142.0 / 2.0)
acc = acc + ((141.0 - k) * (142.0 + 143.0) / 144.0)
acc = acc - (142.0 * k + 143.0 - 144.0 / 3.0)
acc = acc + ((143.0 + 144.0) / k + 145.0 * 2.0)
acc = acc + (144.0 + 145.0 * k - 146.0 / 2.0)
acc = acc + ((145.0 - k) * (146.0 + 147.0) / 148.0)
acc = acc - (146.0 * k + 147.0 - 148.0 / 3.0)
acc = acc + ((147.0 + 148.0) / k + 149.0 * 2.0)
acc = acc + (148.0 + 149.0 * k - 150.0 / 2.0)
acc = acc + ((149.0 - k) * (150.0 + 151.0) / 152.0)
acc = acc - (150.0 * k + 151.0 - 152.0 / 3.0)
acc = acc + ((151.0 + 152.0) / k + 153.0 * 2.0)
acc = acc + (152.0 + 153.0 * k - 154.0 / 2.0)
acc = acc + ((153.0 - k) * (154.0 + 155.0) / 156.0)
acc = acc - (154.0 * k + 155.0 - 156.0 / 3.0)
acc = acc + ((155.0 + 156.0) / k + 157.0 * 2.0)
acc = acc + (156.0 + 157.0 * k - 158.0 / 2.0)
acc = acc + ((157.0 - k) * (158.0 + 159.0) / 160.0)
acc = acc - (158.0 * k + 159.0 - 160.0 / 3.0)
acc = acc + ((159.0 + 160.0) / k + 161.0 * 2.0)
acc = acc + (160.0 + 161.0 * k - 162.0 / 2.0)
acc = acc + ((161.0 - k) * (162.0 + 163.0) / 164.0)
acc = acc - (162.0 * k + 163.0 - 164.0 / 3.0)
acc = acc + ((163.0 + 164.0) / k + 165.0 * 2.0)
acc = acc + (164.0 + 165.0 * k - 166.0 / 2.0)
acc = acc + ((165.0 - k) * (166.0 + 167.0) / 168.0)
acc = acc - (166.0 * k + 167.0 - 168.0 / 3.0)
acc = acc + ((167.0 + 168.0) / k + 169.0 * 2.0)
acc = acc + (168.0 + 169.0 * k - 170.0 / 2.0)
acc = acc + ((169.0 - k) * (170.0 + 171.0) / 172.0)
acc = acc - (170.0 * k + 171.0 - 172.0 / 3.0)
acc = acc + ((171.0 + 172.0) / k + 173.0 * 2.0)
acc = acc + (172.0 + 173.0 * k - 174.0 / 2.0)
acc = acc + ((173.0 - k) * (174.0 + 175.0) / 176.0)
acc = acc - (174.0 * k + 175.0 - 176.0 / 3.0)
acc = acc + ((175.0 + 176.0) / k + 177.0 * 2.0)
acc = acc + (176.0 + 177.0 * k - 178.0 / 2.0)
acc = acc + ((177.0 - k) * (178.0 + 179.0) / 180.0)
acc = acc - (178.0 * k + 179.0 - 180.0 / 3.0)
acc = acc + ((179.0 + 180.0) / k + 181.0 * 2.0)
acc = acc + (180.0 + 181.0 * k - 182.0 / 2.0)
acc = acc + ((181.0 - k) * (182.0 + 183.0) / 184.0)
acc = acc - (182.0 * k + 183.0 - 184.0 / 3.0)
acc = acc + ((183.0 + 184.0) / k + 185.0 * 2.0)
acc = acc + (184.0 + 185.0 * k - 186.0 / 2.0)
acc = acc + ((185.0 - k) * (186.0 + 187.0) / 188.0)
acc = acc - (186.0 * k + 187.0 - 188.0 / 3.0)
acc = acc + ((187.0 + 188.0) / k + 189.0 * 2.0)
acc = acc + (188.0 + 189.0 * k - 190.0 / 2.0)
acc = acc + ((189.0 - k) * (190.0 + 191.0) / 192.0)
acc = acc - (190.0 * k + 191.0 - 192.0 / 3.0)
acc = acc + ((191.0 + 192.0) / k + 193.0 * 2.0)
acc = acc + (192.0 + 193.0 * k - 194.0 / 2.0)
acc = acc + ((193.0 - k) * (194.0 + 195.0) / 196.0)
acc = acc - (194.0 * k + 195.0 - 196.0 / 3.0)
acc = acc + ((195.0 + 196.0) / k + 197.0 * 2.0)
acc = acc + (196.0 + 197.0 * k - 198.0 / 2.0)
acc = acc + ((197.0 - k) * (198.0 + 199.0) / 200.0)
acc = acc - (198.0 * k + 199.0 - 200.0 / 3.0)
acc = acc + ((199.0 + 200.0) / k + 201.0 * 2.0)
acc = acc + (200.0 + 201.0 * k - 202.0 / 2.0)
acc = acc + ((201.0 - k) * (202.0 + 203.0) / 204.0)
acc = acc - (202.0 * k + 203.0 - 204.0 / 3.0)
acc = acc + ((203.0 + 204.0) / k + 205.0 * 2.0)
acc = acc + (204.0 + 205.0 * k - 206.0 / 2.0)
acc = acc + ((205.0 - k) * (206.0 + 207.0) / 208.0)
acc = acc - (206.0 * k + 207.0 - 208.0 / 3.0)
acc = acc + ((207.0 + 208.0) / k + 209.0 * 2.0)
acc = acc + (208.0 + 209.0 * k - 210.0 / 2.0)
acc = acc + ((209.0 - k) * (210.0 + 211.0) / 212.0)
acc = acc - (210.0 * k + 211.0 - 212.0 / 3.0)
acc = acc + ((211.0 + 212.0) / k + 213.0 * 2.0)
acc = acc + (212.0 + 213.0 * k - 214.0 / 2.0)
acc = acc + ((213.0 - k) * (214.0 + 215.0) / 216.0)
acc = acc - (214.0 * k + 215.0 - 216.0 / 3.0)
acc = acc + ((215.0 + 216.0) / k + 217.0 * 2.0)
acc = acc + (216.0 + 217.0 * k - 218.0 / 2.0)
acc = acc + ((217.0 - k) * (218.0 + 219.0) / 220.0)
acc = acc - (218.0 * k + 219.0 - 220.0 / 3.0)
acc = acc + ((219.0 + 220.0) / k + 221.0 * 2.0)
acc = acc + (220.0 + 221.0 * k - 222.0 / 2.0)
acc = acc + ((221.0 - k) * (222.0 + 223.0) / 224.0)
acc = acc - (222.0 * k + 223.0 - 224.0 / 3.0)
acc = acc + ((223.0 + 224.0) / k + 225.0 * 2.0)
acc = acc + (224.0 + 225.0 * k - 226.0 / 2.0)
acc = acc + ((225.0 - k) * (226.0 + 227.0) / 228.0)
acc = acc - (226.0 * k + 227.0 - 228.0 / 3.0)
acc = acc + ((227.0 + 228.0) / k + 229.0 * 2.0)
acc = acc + (228.0 + 229.0 * k - 230.0 / 2.0)
acc = acc + ((229.0 - k) * (230.0 + 231.0) / 232.0)
acc = acc - (230.0 * k + 231.0 - 232.0 / 3.0)
acc = acc + ((231.0 + 232.0) / k + 233.0 * 2.0)
acc = acc + (232.0 + 233.0 * k - 234.0 / 2.0)
acc = acc + ((233.0 - k) * (234.0 + 235.0) / 236.0)
acc = acc - (234.0 * k + 235.0 - 236.0 / 3.0)
acc = acc + ((235.0 + 236.0) / k + 237.0 * 2.0)
acc = acc + (236.0 + 237.0 * k - 238.0 / 2.0)
acc = acc + ((237.0 - k) * (238.0 + 239.0) / 240.0)
acc = acc - (238.0 * k + 239.0 - 240.0 / 3.0)
acc = acc + ((239.0 + 240.0) / k + 241.0 * 2.0)
acc = acc + (240.0 + 241.0 * k - 242.0 / 2.0)
acc = acc + ((241.0 - k) * (242.0 + 243.0) / 244.0)
acc = acc - (242.0 * k + 243.0 - 244.0 / 3.0)
acc = acc + ((243.0 + 244.0) / k + 245.0 * 2.0)
acc = acc + (244.0 + 245.0 * k - 246.0 / 2.0)
acc = acc + ((245.0 - k) * (246.0 + 247.0) / 248.0)
acc = acc - (246.0 * k + 247.0 - 248.0 / 3.0)
acc = acc + ((247.0 + 248.0) / k + 249.0 * 2.0)
acc = acc + (248.0 + 249.0 * k - 250.0 / 2.0)
acc = acc + ((249.0 - k) * (250.0 + 251.0) / 252.0)
acc = acc - (250.0 * k + 251.0 - 252.0 / 3.0)
acc = acc + ((251.0 + 252.0) / k + 253.0 * 2.0)
acc = acc + (252.0 + 253.0 * k - 254.0 / 2.0)
acc = acc + ((253.0 - k) * (254.0 + 255.0) / 256.0)
acc = acc - (254.0 * k + 255.0 - 256.0 / 3.0)
acc = acc + ((255.0 + 256.0) / k + 257.0 * 2.0)
acc = acc + (256.0 + 257.0 * k - 258.0 / 2.0)
acc = acc + ((257.0 - k) * (258.0 + 259.0) / 260.0)
acc = acc - (258.0 * k + 259.0 - 260.0 / 3.0)
acc = acc + ((259.0 + 260.0) / k + 261.0 * 2.0)
acc = acc + (260.0 + 261.0 * k - 262.0 / 2.0)
acc = acc + ((261.0 - k) * (262.0 + 263.0) / 264.0)
acc = acc - (262.0 * k + 263.0 - 264.0 / 3.0)
acc = acc + ((263.0 + 264.0) / k + 265.0 * 2.0)
acc = acc + (264.0 + 265.0 * k - 266.0 / 2.0)
acc = acc + ((265.0 - k) * (266.0 + 267.0) / 268.0)
acc = acc - (266.0 * k + 267.0 - 268.0 / 3.0)
acc = acc + ((267.0 + 268.0) / k + 269.0 * 2.0)
acc = acc + (268.0 + 269.0 * k - 270.0 / 2.0)
acc = acc + ((269.0 - k) * (270.0 + 271.0) / 272.0)
acc = acc - (270.0 * k + 271.0 - 272.0 / 3.0)
acc = acc + ((271.0 + 272.0) / k + 273.0 * 2.0)
acc = acc + (272.0 + 273.0 * k - 274.0 / 2.0)
acc = acc + ((273.0 - k) * (274.0 + 275.0) / 276.0)
acc = acc - (274.0 * k + 275.0 - 276.0 / 3.0)
acc = acc + ((275.0 + 276.0) / k + 277.0 * 2.0)
acc = acc + (276.0 + 277.0 * k - 278.0 / 2.0)
acc = acc + ((277.0 - k) * (278.0 + 279.0) / 280.0)
acc = acc - (278.0 * k + 279.0 - 280.0 / 3.0)
acc = acc + ((279.0 + 280.0) / k + 281.0 * 2.0)
acc = acc + (280.0 + 281.0 * k - 282.0 / 2.0)
acc = acc + ((281.0 - k) * (282.0 + 283.0) / 284.0)
acc = acc - (282.0 * k + 283.0 - 284.0 / 3.0)
acc = acc + ((283.0 + 284.0) / k + 285.0 * 2.0)
acc = acc + (284.0 + 285.0 * k - 286.0 / 2.0)
acc = acc + ((285.0 - k) * (286.0 + 287.0) / 288.0)
acc = acc - (286.0 * k + 287.0 - 288.0 / 3.0)
acc = acc + ((287.0 + 288.0) / k + 289.0 * 2.0)
acc = acc + (288.0 + 289.0 * k - 290.0 / 2.0)
acc = acc + ((289.0 - k) * (290.0 + 291.0) / 292.0)
acc = acc - (290.0 * k + 291.0 - 292.0 / 3.0)
acc = acc + ((291.0 + 292.0) / k + 293.0 * 2.0)
acc = acc + (292.0 + 293.0 * k - 294.0 / 2.0)
acc = acc + ((293.0 - k) * (294.0 + 295.0) / 296.0)
acc = acc - (294.0 * k + 295.0 - 296.0 / 3.0)
acc = acc + ((295.0 + 296.0) / k + 297.0 * 2.0)
acc = acc + (296.0 + 297.0 * k - 298.0 / 2.0)
acc = acc + ((297.0 - k) * (298.0 + 299.0) / 300.0)
acc = acc - (298.0 * k + 299.0 - 300.0 / 3.0)
acc = acc + ((299.0 + 300.0) / k + 301.0 * 2.0)
acc = acc + (300.0 + 301.0 * k - 302.0 / 2.0)
acc = acc + ((301.0 - k) * (302.0 + 303.0) / 304.0)
acc = acc - (302.0 * k + 303.0 - 304.0 / 3.0)
acc = acc + ((303.0 + 304.0) / k + 305.0 * 2.0)
acc = acc + (304.0 + 305.0 * k - 306.0 / 2.0)
acc = acc + ((305.0 - k) * (306.0 + 307.0) / 308.0)
acc = acc - (306.0 * k + 307.0 - 308.0 / 3.0)
acc = acc + ((307.0 + 308.0) / k + 309.0 * 2.0)
acc = acc + (308.0 + 309.0 * k - 310.0 / 2.0)
acc = acc + ((309.0 - k) * (310.0 + 311.0) / 312.0)
acc = acc - (310.0 * k + 311.0 - 312.0 / 3.0)
acc = acc + ((311.0 + 312.0) / k + 313.0 * 2.0)
acc = acc + (312.0 + 313.0 * k - 314.0 / 2.0)
acc = acc + ((313.0 - k) * (314.0 + 315.0) / 316.0)
acc = acc - (314.0 * k + 315.0 - 316.0 / 3.0)
acc = acc + ((315.0 + 316.0) / k + 317.0 * 2.0)
acc = acc + (316.0 + 317.0 * k - 318.0 / 2.0)
acc = acc + ((317.0 - k) * (318.0 + 319.0) / 320.0)
acc = acc - (318.0 * k + 319.0 - 320.0 / 3.0)
acc = acc + ((319.0 + 320.0) / k + 321.0 * 2.0)
acc = acc + (320.0 + 321.0 * k - 322.0 / 2.0)
acc = acc + ((321.0 - k) * (322.0 + 323.0) / 324.0)
acc = acc - (322.0 * k + 323.0 - 324.0 / 3.0)
acc = acc + ((323.0 + 324.0) / k + 325.0 * 2.0)
acc = acc + (324.0 + 325.0 * k - 326.0 / 2.0)
acc = acc + ((325.0 - k) * (326.0 + 327.0) / 328.0)
acc = acc - (326.0 * k + 327.0 - 328.0 / 3.0)
acc = acc + ((327.0 + 328.0) / k + 329.0 * 2.0)
acc = acc + (328.0 + 329.0 * k - 330.0 / 2.0)
acc = acc + ((329.0 - k) * (330.0 + 331.0) / 332.0)
acc = acc - (330.0 * k + 331.0 - 332.0 / 3.0)
acc = acc + ((331.0 + 332.0) / k + 333.0 * 2.0)
acc = acc + (332.0 + 333.0 * k - 334.0 / 2.0)
acc = acc + ((333.0 - k) * (334.0 + 335.0) / 336.0)
acc = acc - (334.0 * k + 335.0 - 336.0 / 3.0)
acc = acc + ((335.0 + 336.0) / k + 337.0 * 2.0)
acc = acc + (336.0 + 337.0 * k - 338.0 / 2.0)
acc = acc + ((337.0 - k) * (338.0 + 339.0) / 340.0)
acc = acc - (338.0 * k + 339.0 - 340.0 / 3.0)
acc = acc + ((339.0 + 340.0) / k + 341.0 * 2.0)
acc = acc + (340.0 + 341.0 * k - 342.0 / 2.0)
acc = acc + ((341.0 - k) * (342.0 + 343.0) / 344.0)
acc = acc - (342.0 * k + 343.0 - 344.0 / 3.0)
acc = acc + ((343.0 + 344.0) / k + 345.0 * 2.0)
acc = acc + (344.0 + 345.0 * k - 346.0 / 2.0)
acc = acc + ((345.0 - k) * (346.0 + 347.0) / 348.0)
acc = acc - (346.0 * k + 347.0 - 348.0 / 3.0)
acc = acc + ((347.0 + 348.0) / k + 349.0 * 2.0)
acc = acc + (348.0 + 349.0 * k - 350.0 / 2.0)
acc = acc + ((349.0 - k) * (350.0 + 351.0) / 352.0)
acc = acc - (350.0 * k + 351.0 - 352.0 / 3.0)
acc = acc + ((351.0 + 352.0) / k + 353.0 * 2.0)
acc = acc + (352.0 + 353.0 * k - 354.0 / 2.0)
acc = acc + ((353.0 - k) * (354.0 + 355.0) / 356.0)
acc = acc - (354.0 * k + 355.0 - 356.0 / 3.0)
acc = acc + ((355.0 + 356.0) / k + 357.0 * 2.0)
acc = acc + (356.0 + 357.0 * k - 358.0 / 2.0)
acc = acc + ((357.0 - k) * (358.0 + 359.0) / 360.0)
acc = acc - (358.0 * k + 359.0 - 360.0 / 3.0)
acc = acc + ((359.0 + 360.0) / k + 361.0 * 2.0)
acc = acc + (360.0 + 361.0 * k - 362.0 / 2.0)
acc = acc + ((361.0 - k) * (362.0 + 363.0) / 364.0)
acc = acc - (362.0 * k + 363.0 - 364.0 / 3.0)
acc = acc + ((363.0 + 364.0) / k + 365.0 * 2.0)
acc = acc + (364.0 + 365.0 * k - 366.0 / 2.0)
acc = acc + ((365.0 - k) * (366.0 + 367.0) / 368.0)
acc = acc - (366.0 * k + 367.0 - 368.0 / 3.0)
acc = acc + ((367.0 + 368.0) / k + 369.0 * 2.0)
acc = acc + (368.0 + 369.0 * k - 370.0 / 2.0)
acc = acc + ((369.0 - k) * (370.0 + 371.0) / 372.0)
acc = acc - (370.0 * k + 371.0 - 372.0 / 3.0)
acc = acc + ((371.0 + 372.0) / k + 373.0 * 2.0)
acc = acc + (372.0 + 373.0 * k - 374.0 / 2.0)
acc = acc + ((373.0 - k) * (374.0 + 375.0) / 376.0)
acc = acc - (374.0 * k + 375.0 - 376.0 / 3.0)
acc = acc + ((375.0 + 376.0) / k + 377.0 * 2.0)
acc = acc + (376.0 + 377.0 * k - 378.0 / 2.0)
acc = acc + ((377.0 - k) * (378.0 + 379.0) / 380.0)
acc = acc - (378.0 * k + 379.0 - 380.0 / 3.0)
acc = acc + ((379.0 + 380.0) / k + 381.0 * 2.0)
acc = acc + (380.0 + 381.0 * k - 382.0 / 2.0)
acc = acc + ((381.0 - k) * (382.0 + 383.0) / 384.0)
acc = acc - (382.0 * k + 383.0 - 384.0 / 3.0)
acc = acc + ((383.0 + 384.0) / k + 385.0 * 2.0)
acc = acc + (384.0 + 385.0 * k - 386.0 / 2.0)
acc = acc + ((385.0 - k) * (386.0 + 387.0) / 388.0)
acc = acc - (386.0 * k + 387.0 - 388.0 / 3.0)
acc = acc + ((387.0 + 388.0) / k + 389.0 * 2.0)
acc = acc + (388.0 + 389.0 * k - 390.0 / 2.0)
acc = acc + ((389.0 - k) * (390.0 + 391.0) / 392.0)
acc = acc - (390.0 * k + 391.0 - 392.0 / 3.0)
acc = acc + ((391.0 + 392.0) / k + 393.0 * 2.0)
acc = acc + (392.0 + 393.0 * k - 394.0 / 2.0)
acc = acc + ((393.0 - k) * (394.0 + 395.0) / 396.0)
acc = acc - (394.0 * k + 395.0 - 396.0 / 3.0)
acc = acc + ((395.0 + 396.0) / k + 397.0 * 2.0)
acc = acc + (396.0 + 397.0 * k - 398.0 / 2.0)
acc = acc + ((397.0 - k) * (398.0 + 399.0) / 400.0)
acc = acc - (398.0 * k + 399.0 - 400.0 / 3.0)
acc = acc + ((399.0 + 400.0) / k + 401.0 * 2.0)
acc = acc + (400.0 + 401.0 * k - 402.0 / 2.0)

-- Direct calls into every generated helper.
acc = acc + helper_00(0.0)
acc = acc + helper_01(1.0)
acc = acc + helper_02(2.0)
acc = acc + helper_03(3.0)
acc = acc + helper_04(4.0)
acc = acc + helper_05(5.0)
acc = acc + helper_06(6.0)
acc = acc + helper_07(7.0)
acc = acc + helper_08(8.0)
acc = acc + helper_09(9.0)
acc = acc + helper_10(10.0)
acc = acc + helper_11(11.0)
acc = acc + helper_12(12.0)
acc = acc + helper_13(13.0)
acc = acc + helper_14(14.0)
acc = acc + helper_15(15.0)
acc = acc + helper_16(16.0)
acc = acc + helper_17(17.0)
acc = acc + helper_18(18.0)
acc = acc + helper_19(19.0)
acc = acc + helper_20(20.0)
acc = acc + helper_21(21.0)
acc = acc + helper_22(22.0)
acc = acc + helper_23(23.0)
acc = acc + helper_24(24.0)
acc = acc + helper_25(25.0)
acc = acc + helper_26(26.0)
acc = acc + helper_27(27.0)
acc = acc + helper_28(28.0)
acc = acc + helper_29(29.0)
acc = acc + helper_30(30.0)
acc = acc + helper_31(31.0)
acc = acc + helper_32(32.0)
acc = acc + helper_33(33.0)
acc = acc + helper_34(34.0)
acc = acc + helper_35(35.0)
acc = acc + helper_36(36.0)
acc = acc + helper_37(37.0)
acc = acc + helper_38(38.0)
acc = acc + helper_39(39.0)
acc = acc + helper_40(40.0)
acc = acc + helper_41(41.0)
acc = acc + helper_42(42.0)
acc = acc + helper_43(43.0)
acc = acc + helper_44(44.0)
acc = acc + helper_45(45.0)
acc = acc + helper_46(46.0)
acc = acc + helper_47(47.0)
acc = acc + helper_48(48.0)
acc = acc + helper_49(49.0)
acc = acc + helper_50(50.0)
acc = acc + helper_51(51.0)
acc = acc + helper_52(52.0)
acc = acc + helper_53(53.0)
acc = acc + helper_54(54.0)
acc = acc + helper_55(55.0)
acc = acc + helper_56(56.0)
acc = acc + helper_57(57.0)
acc = acc + helper_58(58.0)
acc = acc + helper_59(59.0)
acc = acc + helper_60(60.0)
acc = acc + helper_61(61.0)
acc = acc + helper_62(62.0)
acc = acc + helper_63(63.0)

-- Branch-heavy loop with logical operators and elseif chains.
for i = 1, 200 do
    acc = acc + mix(i, i + 1, k)
    if i < 20 then
        hits = hits + 1
    elseif i < 100 then
        hits = hits + 2
    elseif i < 180 then
        hits = hits + 3
    else
        hits = hits + 4
    end
    if i == 25 or i == 75 or i == 150 then
        hits = hits + 10
    end
    if not (i > 195) and flag then
        hits = hits + 1
    end
end

-- While loop with a mid-loop break.
local w = 0
while w < 500 do
    w = w + 1
    acc = acc + abs_value(k - w)
    if w > 480 then
        hits = hits + 1
        break
    end
end

-- Repeat loop.
local r = 0
repeat
    r = r + 1
    acc = acc + r
until r >= 40

-- Descending numeric for loop.
for j = 100, 1, -3 do
    acc = acc + j
end

-- String and boolean work.
STRING_LENGTH = 23
for i = 1, 100 do
    if same_string(s, s) then
        hits = hits + 1
    end
    if same_string(s, t) then
        hits = hits + 1000
    end
    if #s == STRING_LENGTH then
        hits = hits + 1
    end
    if #t > 1 and not flag2 then
        hits = hits + 1
    end
end

-- Recursion and number results.
acc = acc + fib(12)
acc = acc + fact(8)
acc = acc + gcd(1071, 462)
acc = acc + sum_to(50)
acc = acc + clamp_value(k - 500, 1.0, 9.0)
acc = acc + pick(flag, 17.0, 29.0)
if is_prime(97) and flag then
    hits = hits + 100
end
if divisible(360, 15) then
    hits = hits + 100
end

print(acc, hits, type(s), #s, flag and not flag2)
print(fib(10), gcd(48, 18), is_prime(31))
