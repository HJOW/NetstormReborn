// 31h 다리 붕괴 주기 명령을 현재 Session의 스캔 상태와 시계에 연결한다.
import { parseDeveloperCommand } from './raw-campaign-developer-settings.js';
import { resetStateBridgeDecayScan } from './raw-state-bridge-reset.js';

// 양의 정수 비율을 IEEE 단정도로 한 번만 반올림한다. 중간 double 변환은 사용하지 않는다.
function ratioToFloat(sign, numerator, denominator) {
    if (numerator === 0n) return sign * 0;
    let exponent = numerator.toString(2).length - denominator.toString(2).length;
    if (exponent >= 0 ? numerator < (denominator << BigInt(exponent)) : (numerator << BigInt(-exponent)) < denominator) exponent -= 1;
    if (exponent > 127) return sign * Infinity;
    if (exponent < -150) return sign * 0;
    const shift = exponent >= -126 ? 23 - exponent : 149;
    const dividend = shift >= 0 ? numerator << BigInt(shift) : numerator;
    const divisor = shift >= 0 ? denominator : denominator << BigInt(-shift);
    let mantissa = dividend / divisor; const remainder = dividend % divisor;
    if (remainder * 2n > divisor || (remainder * 2n === divisor && (mantissa & 1n) !== 0n)) mantissa += 1n;
    let bits;
    if (exponent < -126) bits = Number(mantissa);
    else {
        if (mantissa === 0x1000000n) { mantissa >>= 1n; exponent += 1; }
        if (exponent > 127) return sign * Infinity;
        bits = ((exponent + 127) << 23) | Number(mantissa - 0x800000n);
    }
    const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, (bits | (sign < 0 ? 0x80000000 : 0)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 10진 또는 16진 유효숫자와 지수를 정확한 정수 비율로 바꾸어 단정도에 저장한다.
function numericPeriod(unsigned, sign) {
    const hexadecimal = /^0x/i.test(unsigned), source = hexadecimal ? unsigned.slice(2) : unsigned;
    const [digits, power = '0'] = source.split(hexadecimal ? /p/i : /e/i), [whole, fraction = ''] = digits.split('.');
    const significant = ((whole || '0') + fraction).replace(/^0+/, '');
    if (!significant) return sign * 0;
    const powerValue = Number(power), scale = powerValue - fraction.length * (hexadecimal ? 4 : 1);
    if (hexadecimal) {
        const first = Number.parseInt(significant[0], 16), bits = (significant.length - 1) * 4 + Math.floor(Math.log2(first)) + 1;
        const exponent = bits - 1 + scale;
        if (exponent > 127) return sign * Infinity;
        if (exponent < -150) return sign * 0;
        const value = BigInt(`0x${significant}`);
        return scale >= 0 ? ratioToFloat(sign, value << BigInt(scale), 1n) : ratioToFloat(sign, value, 1n << BigInt(-scale));
    }
    const magnitude = significant.length - 1 + scale;
    if (magnitude > 38) return sign * Infinity;
    if (magnitude < -46) return sign * 0;
    const value = BigInt(significant);
    return scale >= 0 ? ratioToFloat(sign, value * (10n ** BigInt(scale)), 1n) : ratioToFloat(sign, value, 10n ** BigInt(-scale));
}

// 원문의 %f 입력 접두를 단정도로 변환한다. 입력이 없으면 미정 스택을 합성하지 않는다.
function readPeriod(rest) {
    const match = /^[\t\n\v\f\r ]*([+-]?(?:inf(?:inity)?|nan(?:\([a-z0-9_]*\))?|0x(?:[0-9a-f]+(?:\.[0-9a-f]*)?|\.[0-9a-f]+)(?:p[+-]?\d+)?|(?:\d+(?:\.\d*)?|\.\d+)(?:e[+-]?\d+)?))/i.exec(rest);
    if (!match) throw new TypeError('개발자 다리 스캔 주기의 원문 미정 입력');
    const token = match[1], sign = token.startsWith('-') ? -1 : 1, unsigned = token.replace(/^[+-]/, '');
    const trailing = rest.slice(match[0].length);
    if ((/^0$/i.test(unsigned) && /^x/i.test(trailing)) || (/^0x/i.test(unsigned) && /^p/i.test(trailing)) || (!/^0x/i.test(unsigned) && /^(?:\d|\.)/.test(unsigned) && /^e/i.test(trailing)) || (/^inf$/i.test(unsigned) && /^i/i.test(trailing)) || (/^nan$/i.test(unsigned) && /^\(/.test(trailing))) throw new TypeError('개발자 다리 스캔 주기의 원문 미정 입력');
    if (/^inf/i.test(unsigned)) return sign * Infinity;
    if (/^nan/i.test(unsigned)) return sign < 0 ? -NaN : NaN;
    return numericPeriod(unsigned, sign);
}

// 단정도의 실제 이진값을 여섯 소수 자리로 반올림해 C의 고정 소수 출력으로 바꾼다.
function periodText(value) {
    const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true);
    const bits = view.getUint32(0, true), exponent = (bits >>> 23) & 0xff, fraction = bits & 0x7fffff;
    if (Number.isNaN(value)) return bits >>> 31 ? '-nan' : 'nan';
    if (!Number.isFinite(value)) return value < 0 ? '-inf' : 'inf';
    let scaled = BigInt(exponent ? fraction | 0x800000 : fraction) * 1000000n;
    const shift = exponent ? exponent - 150 : -149;
    if (shift >= 0) scaled <<= BigInt(shift);
    else {
        const divisor = 1n << BigInt(-shift), quotient = scaled / divisor, remainder = scaled % divisor;
        scaled = quotient + BigInt(remainder * 2n >= divisor);
    }
    return `${bits >>> 31 ? '-' : ''}${scaled / 1000000n}.${String(scaled % 1000000n).padStart(6, '0')}`;
}

// 주기를 먼저 저장하고 추적 후 현재 주기와 현재 시각을 다시 읽어 같은 스캔 커서를 초기화한다.
export function executeDeveloperBridge(text, storage, trace) {
    const { key, rest } = parseDeveloperCommand(text);
    if (key !== 'setbridgekillperiod' && key !== 'sbkp') return false;
    const period = readPeriod(rest); storage.bridgeDecayPeriod = period;
    const message = `bkPERIOD now ${periodText(period)}\nresetting bridge killer\n`;
    if (message.length >= 512) throw new RangeError('개발자 주기 로그 물리 범위');
    if (typeof trace !== 'function') throw new Error('개발자 명령의 실제 추적 장치 미연결');
    trace(29, message);
    resetStateBridgeDecayScan(storage.bridgeDecayScan, storage.bridgeDecayPeriod, storage.readGraphClock());
    return true;
}
