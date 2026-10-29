#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i=0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {//phase_01
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    /* 从 64 位 challenge 切出三个 32 位字段（对应 p1_compute 里的移位） */
    uint32_t lo  = (uint32_t)(challenge);          // 位 0..31
    uint32_t mid = (uint32_t)(challenge >> 16);    // 位 16..47
    uint32_t hi  = (uint32_t)(challenge >> 32);    // 位 32..63
    /* p1_step1(hi, mid) = (3*hi) ^ (mid + 0x26fb) */
    uint32_t s1 = (3u * hi) ^ (mid + UINT32_C(0x26fb));
    /* p1_step0(lo, hi) = (lo + 4*hi) ^ 0x60e0 */
    uint32_t s0 = (lo + 4u * hi) ^ UINT32_C(0x60e0);
    /* p1_step2(s0, s1) = (3*s0 + s1) ^ 0x9a8e */
    uint32_t answer = (3u * s0 + s1) ^ UINT32_C(0x9a8e);
    values[0] = answer;
    count = 1;
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;
    (void)challenge;
    /* 初值 v0：取 challenge 位32到38，异或 8 */
    values[0] = ((uint32_t)(challenge >> 32) & UINT32_C(0x7F)) ^ UINT32_C(0x08);
    for (uint32_t i = 1; i <= 5; i++){
        uint32_t shift = 5 * (i + 1);/* 10,15,20,25,30 */
        uint32_t t = ((uint32_t)(challenge >> shift) & UINT32_C (0x3F)) ^ i;
        values [i] = 2 * values [i - 1] + t;
    }
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    (void)challenge;
    uint32_t selector = (uint32_t)challenge & UINT32_C(0x7);
    uint32_t v = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 3);
    uint32_t result;
    switch (selector) {
        case 0: result = (4u * v) ^ UINT32_C(0xc13a);        break; /* p3_case_6 */
        case 1: result = (5u * v) + UINT32_C(0xaa78);        break; /* p3_case_1 */
        case 2: result = (5u * v) + UINT32_C(0x4b7cb);       break; /* p3_case_5 */
        case 3: result = (5u * v) + UINT32_C(0x81b0);        break; /* p3_case_4 */
        case 4: result = (v ^ UINT32_C(0x6a4c)) + (v >> 1);  break; /* p3_case_7 */
        case 5: result = (2u * v) ^ UINT32_C(0x57eb);        break; /* p3_case_3 */
        case 6: result = (v ^ UINT32_C(0x6e16)) + (v >> 2);  break; /* p3_case_0 */
        case 7: result = (3u * v) + UINT32_C(0xc2fa);        break; /* p3_case_2 */
        default: result = 0; break;
    }
    values[0] = selector;
    values[1] = result;
    return write_answer(out, cap, values, count);
}

static uint32_t p4_walk(uint32_t target, uint32_t low, uint32_t high, uint32_t key) {
    uint32_t mid = low + (high - low) / 2;
    if (mid == target) {
        return (target + UINT32_C(0x2b)) ^ key;
    }
    uint32_t node = mid + key;
    if (target < mid) {
        uint32_t sub = p4_walk(target, low, mid - 1, key);
        return (3u * sub) ^ node ^ UINT32_C(0x02);
    } else {
        uint32_t sub = p4_walk(target, mid + 1, high, key);
        return (3u * sub) ^ node ^ UINT32_C(0x19);
    }
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t target = (uint32_t)challenge & UINT32_C(0x7f);
    uint32_t key    = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);

    values[0] = p4_walk(target, 0, UINT32_C(0x7f), key);
    return write_answer(out, cap, values, count);
}

struct p5n {
    uint32_t a;
    uint32_t b;
    const struct p5n *next;
};
// 链表：N5 → N10 → N0 → N3 → N4 → N11 → N9 → N8 → N2 → N1 → N7 → N6 → NULL 
static const struct p5n p5n_nodes[12] = {
    { UINT32_C(0x031e), UINT32_C(0x0582), &p5n_nodes[1]  }, // N5  = heads[0] 
    { UINT32_C(0xa307), UINT32_C(0x0443), &p5n_nodes[2]  }, //N10 = heads[1]
    { UINT32_C(0x9dab), UINT32_C(0x081d), &p5n_nodes[3]  }, // N0  = heads[2] 
    { UINT32_C(0x1bda), UINT32_C(0x0717), &p5n_nodes[4]  }, // N3  = heads[3] 
    { UINT32_C(0x4f9d), UINT32_C(0x036f), &p5n_nodes[5]  }, // N4  = heads[4] 
    { UINT32_C(0x20cb), UINT32_C(0x091f), &p5n_nodes[6]  }, // N11 = heads[5] 
    { UINT32_C(0xcea9), UINT32_C(0x0a52), &p5n_nodes[7]  }, // N9  = heads[6] 
    { UINT32_C(0x7b26), UINT32_C(0x017e), &p5n_nodes[8]  }, // N8  = heads[7] 
    { UINT32_C(0xbff2), UINT32_C(0x0656), &p5n_nodes[9]  }, // N2 
    { UINT32_C(0xaa69), UINT32_C(0x0b39), &p5n_nodes[10] }, // N1 
    { UINT32_C(0xbc93), UINT32_C(0x000a), &p5n_nodes[11] }, // N7 
    { UINT32_C(0x022b), UINT32_C(0x0205), NULL            }  // N6 链尾 
};
static const struct p5n *const p5n_heads[8] = {
    &p5n_nodes[0], &p5n_nodes[1], &p5n_nodes[2], &p5n_nodes[3],
    &p5n_nodes[4], &p5n_nodes[5], &p5n_nodes[6], &p5n_nodes[7]
};
static uint32_t p5_traverse(uint64_t challenge, uint32_t *out) {
    uint32_t idx   = (uint32_t)challenge & UINT32_C(0x7);
    uint32_t count = (uint32_t)(challenge >> 3) & UINT32_C(0x3);
    uint32_t mix   = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);

    const struct p5n *p = p5n_heads[idx];
    uint32_t acc = (mix ^ p->a ^ UINT32_C(0xa205)) + p->b;
    *out = p->b;

    for (uint32_t i = 0; i < count; ++i) {
        p = p->next;
        acc ^= p->a;
        acc += p->b;
        *out = p->b;
    }
    return acc;
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    uint32_t last_b;
    uint32_t acc = p5_traverse(challenge, &last_b);
    values[0] = acc;       /* 第1个数：返回值 */
    values[1] = last_b;    /* 第2个数：最后一个节点的 b */
    return write_answer(out, cap, values, count);
}

/* C 语言没有循环移位，先实现两个辅助函数（均为 32 位） */
static uint32_t rol32(uint32_t v, unsigned n) { return (v << n) | (v >> (32 - n)); }
static uint32_t ror32(uint32_t v, unsigned n) { return (v >> n) | (v << (32 - n)); }
//p6_op_0 
static uint32_t p6_op_0(uint32_t v)
{return (2u * v + 0x1267u) ^ (3u * v);}
// p6_op_1 
static uint32_t p6_op_1(uint32_t v)
{return rol32(v, 3) ^ 0xa720u;}
//p6_op_2 
static uint32_t p6_op_2(uint32_t v)
{return (v ^ (v >> 7)) + 0x200bu;}
//p6_op_3 
static uint32_t p6_op_3(uint32_t v)
{return (v + 0x6753u) ^ (v >> 3);}
//p6_op_4
static uint32_t p6_op_4(uint32_t v)
{return ror32(v, 7);}
// p6_op_5
static uint32_t p6_op_5(uint32_t v)
{return (v ^ 0xa1edu) + (v << 7);}

// 0x405dc0 函数指针表（表里的顺序和 op编号顺序不一致)
typedef uint32_t (*op_fn)(uint32_t);
static const op_fn p6_ops[8] = {
    p6_op_5,  /* table[0] 0x405dc0 → 0x4024e0 */
    p6_op_4,  /* table[1] 0x405dc8 → 0x4024d0 */
    p6_op_0,  /* table[2] 0x405dd0 → 0x402470 */
    p6_op_2,  /* table[3] 0x405dd8 → 0x4024a0 */
    p6_op_1,  /* table[4] 0x405de0 → 0x402490 */
    p6_op_2,  /* table[5] 0x405de8 → 0x4024a0 */
    p6_op_3,  /* table[6] 0x405df0 → 0x4024c0 */
    p6_op_5,  /* table[7] 0x405df8 → 0x4024e0 */
};

//p6_dispatch
static uint32_t p6_dispatch(uint64_t x)
{
    uint32_t arg1 = (uint32_t)(x >> 6) ^ (uint32_t)(x >> 32);
    uint32_t idx1 = (uint32_t)(x & 0x7u);
    uint32_t idx2 = (uint32_t)((x >> 3) & 0x7u);

    uint32_t a = p6_ops[idx1](arg1);  /* ① 第一次间接调用 */
    a ^= 0x118fu;                     /* ② 异或密钥 */
    uint32_t b = p6_ops[idx2](a);     /* ③ 第二次间接调用 */
    return b + 0xda01u;               /* ④ 加偏移 */
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap)
{
    uint32_t values[MAX_FIELDS] = {0};
    size_t count =1;
    (void)challenge;
    values[0] = p6_dispatch(challenge);  /* 本关只有 1 个字段 */
    return write_answer(out, cap, values, count);
}

//secret
static uint32_t secret_mix_f(uint32_t x) {
    return ((x >> 3) ^ UINT32_C(0x3d11)) + (x ^ UINT32_C(0xe395));
}
int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    (void)challenge;
    (void)context;
    (void)out;
    (void)cap;
    uint64_t A = challenge; 
    uint64_t B = context;

    uint32_t c     = (uint32_t)B & UINT32_C(0x3);
    uint32_t foldA = (uint32_t)(A >> 32) ^ (uint32_t)A;
    uint32_t foldB = (uint32_t)(B >> 32) ^ (uint32_t)B;
    uint32_t x     = foldA ^ foldB;
    uint32_t r9    = (c * UINT32_C(0xe395)) ^ (uint32_t)(B >> 16);
    uint32_t m     = secret_mix_f(x);
    uint32_t s1    = m ^ r9;
    uint32_t term  = foldB + c + UINT32_C(0x3d11) + (foldA >> 3);
    uint32_t s2    = s1 ^ term;
    uint32_t s0    = c;
    int n = snprintf(out, cap,"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN %u %u %u",s0, s1, s2);
    if (n<0||(size_t)n>=cap) {
        return 1;
    }
    return 0;
}