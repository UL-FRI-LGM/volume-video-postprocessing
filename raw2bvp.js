class FormatMismatchError extends Error {

constructor() {
    super('Format mismatch');
}

}
class DimensionMismatchError extends Error {

constructor() {
    super('Dimension mismatch');
}

}
// #link DimensionMismatchError

class vec {

static const(length, x) { return new Array(length).fill(x); }
static zeros(length)    { return vec.const(length, 0); }
static ones(length)     { return vec.const(length, 1); }

static clone(a) { return [...a]; }
static length(a) { return Math.hypot(...a); }

static unaryOp(a, op) {
    return a.map(op);
}

static binaryOp(a, b, op) {
    if (a.length !== b.length) {
        throw new DimensionMismatchError();
    }
    const out = vec.zeros(a.length);
    for (let i = 0; i < a.length; i++) {
        out[i] = op(a[i], b[i]);
    }
    return out;
}

static all(a)    { return a.every(a => a); }
static any(a)    { return a.some(a => a); }
static none(a)   { return a.every(a => !a); }

static floor(a)  { return vec.unaryOp(a, Math.floor); }
static ceil(a)   { return vec.unaryOp(a, Math.ceil); }
static round(a)  { return vec.unaryOp(a, Math.round); }
static add(a, b) { return vec.binaryOp(a, b, (a, b) => a + b); }
static sub(a, b) { return vec.binaryOp(a, b, (a, b) => a - b); }
static mul(a, b) { return vec.binaryOp(a, b, (a, b) => a * b); }
static div(a, b) { return vec.binaryOp(a, b, (a, b) => a / b); }
static mod(a, b) { return vec.binaryOp(a, b, (a, b) => a % b); }
static min(a, b) { return vec.binaryOp(a, b, Math.min); }
static max(a, b) { return vec.binaryOp(a, b, Math.max); }

static eq(a, b)  { return vec.binaryOp(a, b, (a, b) => a === b); }
static neq(a, b) { return vec.binaryOp(a, b, (a, b) => a !== b); }
static approx(a, b, eps) { return vec.binaryOp(a, b, (a, b) => Math.abs(a - b < eps)); }
static lt(a, b)  { return vec.binaryOp(a, b, (a, b) => a < b); }
static gt(a, b)  { return vec.binaryOp(a, b, (a, b) => a > b); }
static leq(a, b) { return vec.binaryOp(a, b, (a, b) => a <= b); }
static geq(a, b) { return vec.binaryOp(a, b, (a, b) => a >= b); }

static mulElements(a) {
    return a.reduce((x, y) => x * y);
}

static sumElements(a) {
    return a.reduce((x, y) => x + y);
}

static dot(a, b) {
    return vec.sumElements(vec.mul(a, b));
}

static linearIndex(index, dimensions) {
    const dims = vec.clone(dimensions);
    let scale = 1;
    for (let i = 0; i < dims.length; i++) {
        dims[i] = scale;
        scale *= dimensions[i];
    }
    return vec.sumElements(vec.mul(index, dims));
}

static *lexi(a) {
    const b = new Array(a.length).fill(0);
    const count = a.reduce((a, b) => a * b);
    for (let j = 0; j < count; j++) {
        yield [...b];
        for (let i = 0; i < b.length; i++) {
            b[i]++;
            if (b[i] >= a[i]) {
                b[i] = 0;
            } else {
                break;
            }
        }
    }
}

}
class NotImplementedError extends Error {

constructor() {
    super('Not implemented');
}

}
// #link /utils/NotImplementedError
// #link /utils/vec

class AbstractFormat {

allocateMicroblocks(microblockCount) {
    if (microblockCount < 0) {
        throw new Error('Negative microblock count');
    }

    return new ArrayBuffer(this.microblockSize * microblockCount);
}

allocateBytes(bytes) {
    if (bytes < 0) {
        throw new Error('Negative bytes');
    }
    if (bytes % this.microblockSize !== 0) {
        throw new Error('Not an integer number of microblocks');
    }

    const microblockCount = bytes / this.microblockSize;
    return this.allocateMicroblocks(microblockCount);
}

allocateDimensions(dimensions) {
    if (vec.any(vec.lt(dimensions, vec.zeros(dimensions.length)))) {
        throw new Error('Negative dimensions');
    }
    if (vec.any(vec.mod(dimensions, this.microblockDimensions))) {
        throw new Error('Not an integer number of microblocks');
    }

    const microblockCountVec = vec.div(dimensions, this.microblockDimensions);
    const microblockCount = vec.mulElements(microblockCountVec);
    return this.allocateMicroblocks(microblockCount);
}

get family() {
    throw new NotImplementedError();
}

get microblockSize() {
    throw new NotImplementedError();
}

get microblockDimensions() {
    throw new NotImplementedError();
}

}
// #link AbstractFormat

class BitmapFormat extends AbstractFormat {

get family() {
    return 'bitmap';
}

get microblockSize() {
    return 1;
}

get microblockDimensions() {
    return [2, 2, 2];
}

}
// #link AbstractFormat

class MonoFormat extends AbstractFormat {

constructor(count, size, type) {
    super();
    this.count = count;
    this.size = size;
    this.type = type;
}

get family() {
    return 'mono';
}

get microblockSize() {
    return this.count * this.size;
}

get microblockDimensions() {
    return [1, 1, 1];
}

}
// #link AbstractFormat

class ASTCFormat extends AbstractFormat {

constructor(microblockDimensions) {
    super();
    this._microblockDimensions = microblockDimensions;
}

get family() {
    return 'astc';
}

get microblockSize() {
    16;
}

get microblockDimensions() {
    return this._microblockDimensions;
}

}
// #link /utils/FormatMismatchError
// #link /utils/vec
// #link /formats

class Block {

constructor(dimensions, format, data = null) {
    this.dimensions = dimensions;
    this.format = format;

    this.data = data !== null ? data : format.allocateDimensions(dimensions);
}

get(start, end) {
    const extent = vec.sub(end, start);

    if (vec.any(vec.gt(start, end))) {
        throw new Error('Start greater than end');
    }
    if (vec.any(vec.lt(start, vec.zeros(start.length)))) {
        throw new Error('Start out of bounds');
    }
    if (vec.any(vec.gt(end, this.dimensions))) {
        throw new Error('End out of bounds');
    }
    if (vec.any(vec.mod(start, this.format.microblockDimensions))) {
        throw new Error('Not on microblock boundary');
    }
    if (vec.any(vec.mod(extent, this.format.microblockDimensions))) {
        throw new Error('Not an integer number of microblocks');
    }

    const { microblockSize, microblockDimensions } = this.format;
    const microblockStart = vec.div(start, microblockDimensions);
    const microblockEnd = vec.div(end, microblockDimensions);
    const microblockCropExtent = vec.div(extent, microblockDimensions);
    const microblockFullExtent = vec.div(this.dimensions, microblockDimensions);

    const block = new Block(extent, this.format);
    const srcBytes = new Uint8Array(this.data);
    const dstBytes = new Uint8Array(block.data);
    for (const localMicroblockIndex of vec.lexi(microblockCropExtent)) {
        const globalMicroblockIndex = vec.add(localMicroblockIndex, microblockStart);
        const srcMicroblockIndex = vec.linearIndex(globalMicroblockIndex, microblockFullExtent);
        const dstMicroblockIndex = vec.linearIndex(localMicroblockIndex, microblockCropExtent);
        for (let i = 0; i < microblockSize; i++) {
            dstBytes[i + dstMicroblockIndex * microblockSize] = srcBytes[i + srcMicroblockIndex * microblockSize];
        }
    }
    return block;
}

set(offset, block) {
    const start = offset;
    const end = vec.add(offset, block.dimensions);
    const extent = vec.sub(end, start);

    if (this.format !== block.format) {
        throw new FormatMismatchError();
    }
    if (vec.any(vec.gt(start, end))) {
        throw new Error('Start greater than end');
    }
    if (vec.any(vec.lt(start, vec.zeros(start.length)))) {
        throw new Error('Start out of bounds');
    }
    if (vec.any(vec.gt(end, this.dimensions))) {
        throw new Error('End out of bounds');
    }
    if (vec.any(vec.mod(start, this.format.microblockDimensions))) {
        throw new Error('Not on microblock boundary');
    }
    if (vec.any(vec.mod(extent, this.format.microblockDimensions))) {
        throw new Error('Not an integer number of microblocks');
    }

    const { microblockSize, microblockDimensions } = this.format;
    const microblockStart = vec.div(start, microblockDimensions);
    const microblockEnd = vec.div(end, microblockDimensions);
    const microblockCropExtent = vec.div(extent, microblockDimensions);
    const microblockFullExtent = vec.div(this.dimensions, microblockDimensions);

    const srcBytes = new Uint8Array(block.data);
    const dstBytes = new Uint8Array(this.data);
    for (const localMicroblockIndex of vec.lexi(microblockCropExtent)) {
        const globalMicroblockIndex = vec.add(localMicroblockIndex, microblockStart);
        const srcMicroblockIndex = vec.linearIndex(localMicroblockIndex, microblockCropExtent);
        const dstMicroblockIndex = vec.linearIndex(globalMicroblockIndex, microblockFullExtent);
        for (let i = 0; i < microblockSize; i++) {
            dstBytes[i + dstMicroblockIndex * microblockSize] = srcBytes[i + srcMicroblockIndex * microblockSize];
        }
    }
    return this;
}

}
class ParserFactory {

static string(x) { return x; }
static number(x) { return Number(x); }
static bool(x) { return x === 'true'; }
static json(x) { return JSON.parse(x); }
static dimensions(x) { return x.split('x').map(Number); }

};
// #link ParserFactory

class ArgumentParser {

constructor(spec) {
    this.spec = spec;
}

parse(args) {
    const params = {};
    for (let i = 0; i < args.length; i++) {
        const arg = this.spec.find(arg => {
            return `-${arg.short}` === args[i] || `--${arg.long}` === args[i];
        });
        if (!arg) {
            throw new Error(`Unrecognized parameter: ${args[i]}`);
        }
        const parser = ParserFactory[arg.parser];
        if (!parser) {
            throw new Error(`Unrecognized parser: ${arg.parser}`);
        }
        const value = parser(args[++i]);
        params[arg.long] = value;
    }
    for (const arg of this.spec) {
        if (arg.required && !(arg.long in params)) {
            throw new Error(`Missing required parameter: --${arg.long} (-${arg.short})`);
        }
    }
    return params;
}

printShortHelp() {
    for (const arg of this.spec) {
        console.log(`-${arg.short}\t--${arg.long}\t${arg.shortDescription}`);
    }
}

}
// #link /formats

class FormatFactory {

static create(options) {
    switch (options?.family) {
        case 'mono': return FormatFactory.createMono(options);
        case 'astc': return FormatFactory.createASTC(options);
        default: throw new Error(`Unrecognized format family: ${options?.family}`);
    }
}

static createMono(options) {
    const { count, size, type } = options;

    if (count <= 0) {
        throw new Error('Mono component count must be positive');
    }
    if (size <= 0) {
        throw new Error('Mono component size must be positive');
    }
    if (!['u', 'i', 'f'].includes(type)) {
        throw new Error('Mono component type must be u, i, or f');
    }

    return new MonoFormat(count, size, type);
}

static createASTC(options) {
    const { microblockDimensions } = options;

    return new ASTCFormat(microblockDimensions);
}

}
const PRIME32_1 = 2654435761;
const PRIME32_2 = 2246822519;
const PRIME32_3 = 3266489917;
const PRIME32_4 = 668265263;
const PRIME32_5 = 374761393;

/**
 *
 * @param buffer - byte array or string
 * @param seed - optional seed (32-bit unsigned);
 */
function xxHash32(buffer, seed = 0) {
    const b = buffer;

    /*
        Step 1. Initialize internal accumulators
        Each accumulator gets an initial value based on optional seed input. Since the seed is optional, it can be 0.

        ```
            u32 acc1 = seed + PRIME32_1 + PRIME32_2;
            u32 acc2 = seed + PRIME32_2;
            u32 acc3 = seed + 0;
            u32 acc4 = seed - PRIME32_1;
        ```
        Special case : input is less than 16 bytes
        When input is too small (< 16 bytes), the algorithm will not process any stripe. Consequently, it will not
        make use of parallel accumulators.

        In which case, a simplified initialization is performed, using a single accumulator :

        u32 acc  = seed + PRIME32_5;
        The algorithm then proceeds directly to step 4.
    */

    let acc = (seed + PRIME32_5) & 0xffffffff;
    let offset = 0;

    if (b.length >= 16) {
        const accN = [
            (seed + PRIME32_1 + PRIME32_2) & 0xffffffff,
            (seed + PRIME32_2) & 0xffffffff,
            (seed + 0) & 0xffffffff,
            (seed - PRIME32_1) & 0xffffffff,
        ];

        /*
            Step 2. Process stripes
            A stripe is a contiguous segment of 16 bytes. It is evenly divided into 4 lanes, of 4 bytes each.
            The first lane is used to update accumulator 1, the second lane is used to update accumulator 2, and so on.

            Each lane read its associated 32-bit value using little-endian convention.

            For each {lane, accumulator}, the update process is called a round, and applies the following formula :

            ```
            accN = accN + (laneN * PRIME32_2);
            accN = accN <<< 13;
            accN = accN * PRIME32_1;
            ```

            This shuffles the bits so that any bit from input lane impacts several bits in output accumulator.
            All operations are performed modulo 2^32.

            Input is consumed one full stripe at a time. Step 2 is looped as many times as necessary to consume
            the whole input, except the last remaining bytes which cannot form a stripe (< 16 bytes). When that
            happens, move to step 3.
        */

        const b = buffer;
        const limit = b.length - 16;
        let lane = 0;
        for (offset = 0; (offset & 0xfffffff0) <= limit; offset += 4) {
            const i = offset;
            const laneN0 = b[i + 0] + (b[i + 1] << 8);
            const laneN1 = b[i + 2] + (b[i + 3] << 8);
            const laneNP = laneN0 * PRIME32_2 + ((laneN1 * PRIME32_2) << 16);
            let acc = (accN[lane] + laneNP) & 0xffffffff;
            acc = (acc << 13) | (acc >>> 19);
            const acc0 = acc & 0xffff;
            const acc1 = acc >>> 16;
            accN[lane] = (acc0 * PRIME32_1 + ((acc1 * PRIME32_1) << 16)) & 0xffffffff;
            lane = (lane + 1) & 0x3;
        }

        /*
            Step 3. Accumulator convergence
            All 4 lane accumulators from previous steps are merged to produce a single remaining accumulator
            of same width (32-bit). The associated formula is as follows :

            ```
            acc = (acc1 <<< 1) + (acc2 <<< 7) + (acc3 <<< 12) + (acc4 <<< 18);
            ```
        */
        acc =
            (((accN[0] << 1) | (accN[0] >>> 31)) +
                ((accN[1] << 7) | (accN[1] >>> 25)) +
                ((accN[2] << 12) | (accN[2] >>> 20)) +
                ((accN[3] << 18) | (accN[3] >>> 14))) &
            0xffffffff;
    }

    /*
        Step 4. Add input length
        The input total length is presumed known at this stage. This step is just about adding the length to
        accumulator, so that it participates to final mixing.

        ```
        acc = acc + (u32)inputLength;
        ```
    */
    acc = (acc + buffer.length) & 0xffffffff;

    /*
        Step 5. Consume remaining input
        There may be up to 15 bytes remaining to consume from the input. The final stage will digest them according
        to following pseudo-code :
        ```
        while (remainingLength >= 4) {
            lane = read_32bit_little_endian(input_ptr);
            acc = acc + lane * PRIME32_3;
            acc = (acc <<< 17) * PRIME32_4;
            input_ptr += 4; remainingLength -= 4;
        }
        ```
        This process ensures that all input bytes are present in the final mix.
    */

    const limit = buffer.length - 4;
    for (; offset <= limit; offset += 4) {
        const i = offset;
        const laneN0 = b[i + 0] + (b[i + 1] << 8);
        const laneN1 = b[i + 2] + (b[i + 3] << 8);
        const laneP = laneN0 * PRIME32_3 + ((laneN1 * PRIME32_3) << 16);
        acc = (acc + laneP) & 0xffffffff;
        acc = (acc << 17) | (acc >>> 15);
        acc = ((acc & 0xffff) * PRIME32_4 + (((acc >>> 16) * PRIME32_4) << 16)) & 0xffffffff;
    }

    /*
        ```
        while (remainingLength >= 1) {
            lane = read_byte(input_ptr);
            acc = acc + lane * PRIME32_5;
            acc = (acc <<< 11) * PRIME32_1;
            input_ptr += 1; remainingLength -= 1;
        }
        ```
    */

    for (; offset < b.length; ++offset) {
        const lane = b[offset];
        acc = acc + lane * PRIME32_5;
        acc = (acc << 11) | (acc >>> 21);
        acc = ((acc & 0xffff) * PRIME32_1 + (((acc >>> 16) * PRIME32_1) << 16)) & 0xffffffff;
    }

    /*
        Step 6. Final mix (avalanche)
        The final mix ensures that all input bits have a chance to impact any bit in the output digest,
        resulting in an unbiased distribution. This is also called avalanche effect.
        ```
        acc = acc xor (acc >> 15);
        acc = acc * PRIME32_2;
        acc = acc xor (acc >> 13);
        acc = acc * PRIME32_3;
        acc = acc xor (acc >> 16);
        ```
    */

    acc = acc ^ (acc >>> 15);
    acc = (((acc & 0xffff) * PRIME32_2) & 0xffffffff) + (((acc >>> 16) * PRIME32_2) << 16);
    acc = acc ^ (acc >>> 13);
    acc = (((acc & 0xffff) * PRIME32_3) & 0xffffffff) + (((acc >>> 16) * PRIME32_3) << 16);
    acc = acc ^ (acc >>> 16);

    // turn any negatives back into a positive number;
    return acc < 0 ? acc + 4294967296 : acc;
}
const SAF_SIGNATURE = Uint8Array.from([0xab, 0x53, 0x41, 0x46, 0x20, 0x31, 0x30, 0xbb, 0x0d, 0x0a, 0x1a, 0x0a]);

function safEncode(files) {
    const manifest = files.map(({ path, mime, data }) => ({
        path,
        mime,
        size: data.byteLength,
    }));
    const manifestBuffer = new TextEncoder().encode(JSON.stringify(manifest));

    const manifestSizeBuffer = new Uint8Array(4);
    new DataView(manifestSizeBuffer.buffer).setUint32(0, manifestBuffer.byteLength, true);

    const parts = [
        SAF_SIGNATURE,
        manifestSizeBuffer,
        manifestBuffer,
        ...files.map(({ data }) => data),
    ];

    const safSize = parts.reduce((size, { byteLength }) => size + byteLength, 0);
    const saf = new Uint8Array(safSize);
    parts.reduce((saf, part) => (saf.set(part), saf).subarray(part.byteLength), saf);

    return saf;
}

function safDecode(saf) {
    for (let i = 0; i < SAF_SIGNATURE.length; i++) {
        if (saf[i] !== SAF_SIGNATURE[i]) {
            throw new Error('SAF signature mismatch');
        }
    }
    saf = saf.subarray(SAF_SIGNATURE.length);
    const manifestSize = new DataView(saf).getUint32(0, true);
    saf = saf.subarray(4);
    const manifest = JSON.parse(new TextDecoder().decode(saf.subarray(0, manifestSize)));
    saf = saf.subarray(manifestSize);
    return manifest.map(({ path, mime, size }) => ({
        path,
        mime,
        data: (saf = saf.subarray(0, size)),
    }));
}
// #link /volume
// #link /cli/argpar
// #link /cli/FormatFactory
// #link /utils/vec
// #link /utils/xxHash32
// #link /saf

const fs = require('fs');

const argpar = new ArgumentParser([
    {
        short: 'i',
        long: 'input-file',
        shortDescription: 'Path to the input file.',
        parser: 'string',
    },
    {
        short: 'o',
        long: 'output-file',
        shortDescription: 'Path to the output file.',
        parser: 'string',
    },
    {
        short: 'd',
        long: 'dimensions',
        shortDescription: 'Input file dimensions.',
        parser: 'dimensions',
    },
    {
        short: 'bd',
        long: 'block-dimensions',
        shortDescription: 'Block dimensions.',
        parser: 'dimensions',
    },
    {
        short: 'f',
        long: 'input-format',
        shortDescription: 'Input file format.',
        parser: 'json',
    },
]);

const params = argpar.parse(process.argv.slice(2));
const format = FormatFactory.create(params['input-format']);
const dimensions = params['dimensions'];
const blockDimensions = params['block-dimensions'] ?? dimensions;
const buffer = fs.readFileSync(params['input-file']);
const data = new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength);

const parentBlock = new Block(dimensions, format, data);

const manifest = {
    asset: {
        version: '1.0',
    },
    formats: [
        {
            family: format.family,
            microblockSize: format.microblockSize,
            microblockDimensions: format.microblockDimensions,
        },
    ],
    modalities: [
        {
            block: 0,
        },
    ],
    blocks: [
        {
            dimensions: dimensions,
            placements: [],
        },
    ],
};

const manifestFile = {
    path: 'manifest.json',
    mime: 'application/json',
};

const files = [
    manifestFile,
];

const blockHashMap = new Map();

const blockCount = vec.ceil(vec.div(dimensions, blockDimensions));
for (const blockIndex of vec.lexi(blockCount)) {
    const blockStart = vec.mul(blockIndex, blockDimensions);
    const blockEnd = vec.min(vec.add(blockStart, blockDimensions), dimensions);

    const block = parentBlock.get(blockStart, blockEnd);
    const blockHash = xxHash32(new Uint8Array(block.data));

    const blockAlreadyExists = blockHashMap.has(blockHash);

    const blockID = blockAlreadyExists ? blockHashMap.get(blockHash) : manifest.blocks.length;
    const blockURL = `blocks/block${blockID}`;

    manifest.blocks[0].placements.push({
        position: blockStart,
        block: blockID,
    });

    if (!blockAlreadyExists) {
        blockHashMap.set(blockHash, blockID);

        manifest.blocks.push({
            dimensions: block.dimensions,
            format: 0,
            data: blockURL,
        });

        files.push({
            path: blockURL,
            data: new Uint8Array(block.data),
        });
    }

}

manifestFile.data = new TextEncoder().encode(JSON.stringify(manifest));

const saf = safEncode(files);

if (fs.existsSync(params['output-file'])) {
    throw new Error(`Output file ${params['output-file']} exists, refusing to write`);
}

fs.writeFileSync(params['output-file'], saf);

