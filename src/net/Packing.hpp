#pragma once

#include <rva/variant.hpp>

#include <msgpack.hpp>
#include <variant>

namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
    namespace adaptor {

    //-------------------------------------------------------------------------
    // std::monostate packing

    template <>
    struct convert<std::monostate> {
        msgpack::object const &operator()(msgpack::object const &msgpack_o,
                                          std::monostate &msgpack_v) const {
            if (!msgpack_o.is_nil()) {
                throw msgpack::type_error();
            }
            return msgpack_o;
        }
    };

    template <>
    struct pack<std::monostate> {
        template <typename Stream>
        msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &msgpack_o,
                                            const std::monostate &msgpack_v) const {
            msgpack_o.pack_nil();
            return msgpack_o;
        }
    };

    //-------------------------------------------------------------------------
    // rva::variant packing

    template <class... Types>
    struct convert<rva::variant<Types...>> {
        using rva_variant_t = rva::variant<Types...>;
        using variant_t = rva_variant_t::base_type;

        msgpack::object const &operator()(msgpack::object const &msgpack_o,
                                          rva_variant_t &msgpack_v) const {
            return convert<variant_t>{}(msgpack_o, msgpack_v.get_base());
        }
    };

    template <class... Types>
    struct pack<rva::variant<Types...>> {
        using rva_variant_t = rva::variant<Types...>;
        using variant_t = rva_variant_t::base_type;

        template <typename Stream>
        msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &msgpack_o,
                                            const rva_variant_t &msgpack_v) const {
            return pack<variant_t>{}(msgpack_o, msgpack_v.get_base());
        }
    };

    } // namespace adaptor
}

} // namespace msgpack
