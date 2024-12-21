#pragma once
#define REQ_ENUM_BEGIN(n) enum class n##_t: uint8_t {
#define REQ_ENUM_ITEM(prefix, n, v) n = v,
#define REQ_ENUM_END };

template<auto req_a> struct xfer_t {};

#define DEFINE_XFER(type, t_req, t_resp) template<> struct xfer_t<type> {using Treq = t_req; using Tresp = t_resp;};
