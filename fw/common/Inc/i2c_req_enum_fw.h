#pragma once
#define REQ_ENUM_BEGIN(n) enum n##_e {
#define REQ_ENUM_ITEM(prefix, n, v) prefix##_##n = v,
#define REQ_ENUM_END };
#define DEFINE_XFER(type, t_req, t_resp)
