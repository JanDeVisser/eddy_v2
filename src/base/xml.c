/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <base/xml.h>
#include <ctype.h>

typedef struct {
    StringBuilder sb;
    StringBuilder escaped;
    int           indent;
} XMLSerializer;

typedef struct {
    StringScanner ss;
    StringBuilder sb;
} XMLDeserializer;

static XMLNode                   xml_create(XMLNodeImpls *impls, size_t parent, XMLType type);
static XMLNodeImpl              *xml_impl(XMLNode node);
static void                      xml_escape(StringBuilder *sb, StringView text);
static StringView                get_string(XMLNode node, char const *fmt, ...);
static void                      serialize_to_builder(XMLNode node, XMLSerializer *serializer);
static ErrorOrOptionalXMLNode    deserialize_node(XMLDeserializer *deserializer, XMLNode parent);
static ErrorOrOptionalStringView deserialize_text(XMLDeserializer *deserializer, StringView close);
static ErrorOrStringView         deserialize_attr_value(XMLDeserializer *deserializer);
static ErrorOrStringView         deserialize_tag(XMLDeserializer *deserializer);
static ErrorOrOptionalXMLNode    deserialize_processing_instruction(XMLDeserializer *deserializer, XMLNode parent);
static ErrorOrOptionalXMLNode    deserialize_element(XMLDeserializer *deserializer, XMLNode parent);

DA_IMPL(XMLNodeImpl)
DA_IMPL(XMLNode)

extern char const *XMLType_name(XMLType type)
{
    switch (type) {
#undef XMLTYPE
#define XMLTYPE(T)     \
    case XML_TYPE_##T: \
        return #T;
        XMLTYPES(XMLTYPE)
#undef XMLTYPE
    default:
        UNREACHABLE();
    }
}

XMLNode xml_create(XMLNodeImpls *impls, size_t parent, XMLType type)
{
    XMLNode     ret = { impls, impls->size };
    XMLNodeImpl impl = { 0 };
    impl.type = type;
    impl.parent = parent;
    impl.index = ret.index;
    da_append_XMLNodeImpl(impls, impl);
    return ret;
}

XMLNodeImpl *xml_impl(XMLNode node)
{
    assert(node.index < node.registry->size);
    return da_element_XMLNodeImpl(node.registry, node.index);
}

XMLNode xml_document_of(XMLNode node)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl);
    while (impl->type != XML_TYPE_DOCUMENT) {
        impl = xml_impl((XMLNode) { node.registry, impl->parent });
    }
    return (XMLNode) { node.registry, impl->index };
}

void xml_escape(StringBuilder *sb, StringView text)
{
    for (size_t ix = 0; ix < text.length; ++ix) {
        int ch = text.ptr[ix];
        if (ch < ' ' || ch > '~') {
            sb_printf(sb, "&#%d;", ch);
            continue;
        }
        sb_append_char(sb, ch);
    }
}

StringView get_string(XMLNode node, char const *fmt, ...)
{
    XMLNode      doc = xml_document_of(node);
    XMLNodeImpl *impl = xml_impl(doc);
    impl->document.sb.view.length = 0;
    va_list args;
    va_start(args, fmt);
    sb_vprintf(&impl->document.sb, fmt, args);
    va_end(args);
    return impl->document.sb.view;
}

StringView xml_to_string(XMLNode node)
{
    XMLNodeImpl *impl = xml_impl(node);
    switch (impl->type) {
    case XML_TYPE_ELEMENT:
    case XML_TYPE_PROCESSING_INSTRUCTION:
    case XML_TYPE_ATTRIBUTE:
        return impl->element.tag;
    case XML_TYPE_TEXT:
        if (impl->text.length > 20) {
            return get_string(node, "'%.*s...'", impl->text.ptr, 20);
        }
        return get_string(node, "'%.*s'", SV_ARG(impl->text));
    default:
        return sv_from(XMLType_name(impl->type));
    }
}

StringView _xml_debug(XMLNode node, XMLNodeImpl *impl)
{
    switch (impl->type) {
    case XML_TYPE_DOCUMENT:
        return sv_printf("D [%zu][%zu]", impl->document.processing_instructions.size, impl->document.children.size);
    case XML_TYPE_ELEMENT:
        return sv_printf("E %.*s [%zu][%zu]", SV_ARG(impl->element.tag), impl->element.attributes.size, impl->element.children.size);
    case XML_TYPE_PROCESSING_INSTRUCTION:
        return sv_printf("P %.*s [%zu]", SV_ARG(impl->pi.tag), impl->element.attributes.size);
    case XML_TYPE_ATTRIBUTE:
        return sv_printf("A %.*s='%.*s'", SV_ARG(impl->attribute.tag), SV_ARG(impl->attribute.text));
    case XML_TYPE_TEXT:
        if (impl->text.length > 20) {
            return get_string(node, "T '%.*s...'", impl->text.ptr, 20);
        }
        return get_string(node, "T '%.*s'", SV_ARG(impl->text));
    default:
        UNREACHABLE();
    }
}

StringView xml_debug(XMLNode node)
{
    XMLNodeImpl *impl = xml_impl(node);
    StringView   debug = _xml_debug(node, impl);
    StringView   ret = sv_printf("[%zu] %.*s", node.index, SV_ARG(debug));
    sv_free(debug);
    return ret;
}

#define TRACE(node, msg, ...)                                        \
    if (log_category_on(CAT_XML)) {                                  \
        StringView _debug = xml_debug(node);                         \
        StringView _msg = sv_printf(msg __VA_OPT__(, ) __VA_ARGS__); \
        trace(CAT_XML, "%.*s: %.*s", SV_ARG(_debug), SV_ARG(_msg));  \
        sv_free(_msg);                                               \
        sv_free(_debug);                                             \
    }

XMLNode xml_document()
{
    XMLNodeImpls *impls = MALLOC(XMLNodeImpls);
    XMLNode       ret = xml_create(impls, -1, XML_TYPE_DOCUMENT);
    assert(impls->size == 1);

    XMLNode pi = xml_processing_instruction(ret, sv_from("xml"));
    xml_set_attribute(pi, sv_from("version"), sv_from("1.0"));
    TRACE(ret, "Created document");
    return ret;
}

XMLType xml_node_type(XMLNode node)
{
    XMLNodeImpl *impl = xml_impl(node);
    return impl->type;
}

size_t xml_child_count(XMLNode node)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl->type == XML_TYPE_DOCUMENT || impl->type == XML_TYPE_ELEMENT);
    return impl->element.children.size;
}

XMLNode xml_child(XMLNode node, size_t ix)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl->type == XML_TYPE_DOCUMENT || impl->type == XML_TYPE_ELEMENT);
    assert(ix < impl->element.children.size);
    return (XMLNode) { node.registry, impl->element.children.elements[ix] };
}

XMLNodes xml_children_by_tag(XMLNode node, StringView tag)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl->type == XML_TYPE_DOCUMENT || impl->type == XML_TYPE_ELEMENT);
    XMLNodes ret = { 0 };
    for (size_t ix = 0; ix < impl->element.children.size; ++ix) {
        XMLNode      elem = { node.registry, impl->element.children.elements[ix] };
        XMLNodeImpl *elem_impl = xml_impl(elem);
        if (elem_impl->type != XML_TYPE_ELEMENT) {
            continue;
        }
        if (sv_eq(elem_impl->element.tag, tag)) {
            da_append_XMLNode(&ret, elem);
        }
    }
    return ret;
}

OptionalXMLNode xml_first_child_by_tag(XMLNode node, StringView tag)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl->type == XML_TYPE_DOCUMENT || impl->type == XML_TYPE_ELEMENT);
    for (size_t ix = 0; ix < impl->element.children.size; ++ix) {
        XMLNode      elem = { node.registry, impl->element.children.elements[ix] };
        XMLNodeImpl *elem_impl = xml_impl(elem);
        if (elem_impl->type != XML_TYPE_ELEMENT) {
            continue;
        }
        if (sv_eq(elem_impl->element.tag, tag)) {
            RETURN_VALUE(XMLNode, elem);
        }
    }
    RETURN_EMPTY(XMLNode);
}

size_t xml_attribute_count(XMLNode node)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl->type == XML_TYPE_PROCESSING_INSTRUCTION || impl->type == XML_TYPE_ELEMENT);
    return impl->element.attributes.size;
}

XMLNode xml_attribute(XMLNode node, size_t ix)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl->type == XML_TYPE_PROCESSING_INSTRUCTION || impl->type == XML_TYPE_ELEMENT);
    assert(ix < impl->element.attributes.size);
    return (XMLNode) { node.registry, impl->element.attributes.elements[ix] };
}

OptionalXMLNode xml_attribute_by_tag(XMLNode node, StringView tag)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl->type == XML_TYPE_PROCESSING_INSTRUCTION || impl->type == XML_TYPE_ELEMENT);
    for (size_t ix = 0; ix < impl->element.attributes.size; ++ix) {
        XMLNode      attr = { node.registry, impl->element.attributes.elements[ix] };
        XMLNodeImpl *attr_impl = xml_impl(attr);
        if (sv_eq(attr_impl->attribute.tag, tag)) {
            RETURN_VALUE(XMLNode, attr);
        }
    }
    RETURN_EMPTY(XMLNode);
}

StringView xml_tag(XMLNode node)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl->type == XML_TYPE_PROCESSING_INSTRUCTION || impl->type == XML_TYPE_ELEMENT);
    return impl->element.tag;
}

OptionalStringView xml_text_of(XMLNode node)
{
    XMLNodeImpl *impl = xml_impl(node);
    assert(impl->type != XML_TYPE_PROCESSING_INSTRUCTION);
    switch (impl->type) {
    case XML_TYPE_TEXT:
        RETURN_VALUE(StringView, impl->text);
    case XML_TYPE_ATTRIBUTE:
        RETURN_VALUE(StringView, impl->attribute.text);
    case XML_TYPE_ELEMENT:
    case XML_TYPE_DOCUMENT: {
        if (impl->element.children.size == 0 || xml_node_type(xml_child(node, 0)) != XML_TYPE_TEXT) {
            RETURN_EMPTY(StringView);
        }
        return xml_text_of(xml_child(node, 0));
    }
    default:
        UNREACHABLE();
    }
}

XMLNode xml_processing_instruction(XMLNode parent, StringView tag)
{
    XMLNodeImpl *parent_impl = xml_impl(parent);
    assert(parent_impl->type == XML_TYPE_DOCUMENT);
    if (sv_eq_cstr(tag, "xml") && parent_impl->document.processing_instructions.size != 0) {
        return (XMLNode) { parent.registry, parent_impl->document.processing_instructions.elements[0] };
    }
    XMLNode      pi = xml_create(parent.registry, parent.index, XML_TYPE_PROCESSING_INSTRUCTION);
    XMLNodeImpl *pi_impl = xml_impl(pi);
    pi_impl->pi.tag = sv_copy(tag);
    da_append_size_t(&parent_impl->document.processing_instructions, pi.index);
    StringView pi_debug = xml_debug(pi);
    TRACE(parent, "xml_processing_instruction(%.*s)", SV_ARG(pi_debug));
    sv_free(pi_debug);
    return pi;
}

XMLNode xml_element(XMLNode parent, StringView tag)
{
    XMLNodeImpl *parent_impl = xml_impl(parent);
    assert(parent_impl->type == XML_TYPE_DOCUMENT || parent_impl->type == XML_TYPE_ELEMENT);
    assert(xml_child_count(parent) == 0 || xml_node_type(xml_child(parent, 0)) == XML_TYPE_ELEMENT);
    XMLNode      elem = xml_create(parent.registry, parent.index, XML_TYPE_ELEMENT);
    XMLNodeImpl *elem_impl = xml_impl(elem);
    elem_impl->element.tag = sv_copy(tag);
    da_append_size_t(&parent_impl->element.children, elem.index);
    StringView elem_debug = xml_debug(elem);
    TRACE(parent, "xml_element(%.*s)", SV_ARG(elem_debug));
    sv_free(elem_debug);
    return elem;
}

XMLNode xml_text(XMLNode parent, StringView text)
{
    XMLNodeImpl *parent_impl = xml_impl(parent);
    assert(parent_impl->type == XML_TYPE_DOCUMENT || parent_impl->type == XML_TYPE_ELEMENT);
    assert(xml_child_count(parent) == 0);
    XMLNode      txt = xml_create(parent.registry, parent.index, XML_TYPE_TEXT);
    XMLNodeImpl *txt_impl = xml_impl(txt);
    txt_impl->text = sv_copy(text);
    da_append_size_t(&parent_impl->element.children, txt.index);
    StringView debug = xml_debug(txt);
    TRACE(parent, "xml_text(%.*s)", SV_ARG(debug));
    sv_free(debug);
    return txt;
}

XMLNode xml_text_element(XMLNode parent, StringView tag, StringView text)
{
    XMLNode ret = xml_element(parent, tag);
    xml_text(ret, text);
    return ret;
}

void _xml_free(XMLNode node)
{
    XMLNodeImpl *impl = xml_impl(node);
    switch (impl->type) {
    case XML_TYPE_TEXT:
        sv_free(impl->text);
        break;
    case XML_TYPE_ATTRIBUTE:
        sv_free(impl->attribute.tag);
        sv_free(impl->attribute.text);
        break;
    case XML_TYPE_ELEMENT: {
        for (size_t ix = 0; ix < impl->element.attributes.size; ++ix) {
            _xml_free((XMLNode) { node.registry, impl->element.attributes.elements[ix] });
        }
        for (size_t ix = 0; ix < impl->element.children.size; ++ix) {
            _xml_free((XMLNode) { node.registry, impl->element.children.elements[ix] });
        }
        da_free_size_t(&impl->element.attributes);
        da_free_size_t(&impl->element.children);
        sv_free(impl->element.tag);
    } break;
    case XML_TYPE_PROCESSING_INSTRUCTION: {
        for (size_t ix = 0; ix < impl->pi.attributes.size; ++ix) {
            _xml_free((XMLNode) { node.registry, impl->pi.attributes.elements[ix] });
        }
        da_free_size_t(&impl->pi.attributes);
        sv_free(impl->pi.tag);
    } break;
    case XML_TYPE_DOCUMENT: {
        for (size_t ix = 0; ix < impl->document.processing_instructions.size; ++ix) {
            _xml_free((XMLNode) { node.registry, impl->document.processing_instructions.elements[ix] });
        }
        for (size_t ix = 0; ix < impl->element.children.size; ++ix) {
            _xml_free((XMLNode) { node.registry, impl->element.children.elements[ix] });
        }
        da_free_size_t(&impl->document.processing_instructions);
        da_free_size_t(&impl->document.children);
        sv_free(impl->document.sb.view);
        da_free_XMLNodeImpl(node.registry);
        free(node.registry);
    } break;
    }
}

void xml_free(XMLNode doc)
{
    assert(xml_node_type(doc) == XML_TYPE_DOCUMENT);
    _xml_free(doc);
}

XMLNode xml_set_attribute(XMLNode node, StringView attr_tag, StringView text)
{
    XMLNodeImpl *parent_impl = xml_impl(node);
    assert(parent_impl->type == XML_TYPE_PROCESSING_INSTRUCTION || parent_impl->type == XML_TYPE_ELEMENT);
    for (size_t ix = 0; ix < parent_impl->element.attributes.size; ++ix) {
        XMLNodeImpl *attr = xml_impl((XMLNode) { node.registry, parent_impl->element.attributes.elements[ix] });
        if (sv_eq(attr->attribute.tag, attr_tag)) {
            sv_free(attr->attribute.text);
            attr->attribute.text = sv_copy(text);
            TRACE(node, "xml_set_attribute(%.*s, '%.*s') -> overwriting %zu", SV_ARG(attr_tag), SV_ARG(text), parent_impl->element.attributes.size)
            return node;
        }
    }
    XMLNode      a = xml_create(node.registry, node.index, XML_TYPE_ATTRIBUTE);
    XMLNodeImpl *impl = xml_impl(a);
    impl->attribute.tag = sv_copy(attr_tag);
    impl->attribute.text = sv_copy(text);
    da_append_size_t(&parent_impl->element.attributes, a.index);
    TRACE(node, "xml_set_attribute(%.*s, '%.*s') -> new %zu", SV_ARG(attr_tag), SV_ARG(text), parent_impl->element.attributes.size)
    return a;
}

void serialize_to_builder(XMLNode node, XMLSerializer *serializer)
{
    XMLNodeImpl *impl = xml_impl(node);
    switch (impl->type) {
    case XML_TYPE_DOCUMENT: {
        for (size_t ix = 0; ix < impl->document.processing_instructions.size; ix++) {
            XMLNode pi = (XMLNode) { node.registry, impl->document.processing_instructions.elements[ix] };
            serialize_to_builder(pi, serializer);
        }
        for (size_t ix = 0; ix < impl->document.children.size; ix++) {
            XMLNode elem = (XMLNode) { node.registry, impl->document.children.elements[ix] };
            serialize_to_builder(elem, serializer);
        }
    } break;
    case XML_TYPE_ELEMENT: {
        sb_printf(&serializer->sb, "%*s<%.*s", serializer->indent, "", SV_ARG(impl->element.tag));
        for (size_t ix = 0; ix < impl->element.attributes.size; ix++) {
            sb_append_cstr(&serializer->sb, " ");
            XMLNode attr = (XMLNode) { node.registry, impl->element.attributes.elements[ix] };
            serialize_to_builder(attr, serializer);
        }
        if (impl->element.children.size == 0) {
            sb_append_cstr(&serializer->sb, "/>\n");
            break;
        }
        sb_append_char(&serializer->sb, '>');
        if (xml_node_type(xml_child(node, 0)) != XML_TYPE_TEXT) {
            sb_append_char(&serializer->sb, '\n');
        }
        serializer->indent += 4;
        for (size_t ix = 0; ix < impl->element.children.size; ix++) {
            XMLNode elem = (XMLNode) { node.registry, impl->element.children.elements[ix] };
            serialize_to_builder(elem, serializer);
        }
        serializer->indent -= 4;
        sb_printf(&serializer->sb, "</%.*s>\n", SV_ARG(impl->element.tag));
    } break;
    case XML_TYPE_TEXT:
        serializer->escaped.view.length = 0;
        xml_escape(&serializer->escaped, impl->text);
        sb_printf(&serializer->sb, "%.*s", SV_ARG(serializer->escaped.view));
        break;
    case XML_TYPE_ATTRIBUTE:
        serializer->escaped.view.length = 0;
        xml_escape(&serializer->escaped, impl->attribute.text);
        sb_printf(&serializer->sb, "%.*s=\"%.*s\"", SV_ARG(impl->attribute.tag), SV_ARG(serializer->escaped.view));
        break;
    case XML_TYPE_PROCESSING_INSTRUCTION: {
        sb_printf(&serializer->sb, "<?%.*s", SV_ARG(impl->pi.tag));
        for (size_t ix = 0; ix < impl->element.attributes.size; ix++) {
            sb_append_cstr(&serializer->sb, " ");
            XMLNode attr = (XMLNode) { node.registry, impl->pi.attributes.elements[ix] };
            serialize_to_builder(attr, serializer);
        }
        sb_append_cstr(&serializer->sb, "?>\n");
    } break;
    default:
        UNREACHABLE();
    }
}

StringView xml_serialize(XMLNode node)
{
    XMLSerializer serializer = { 0 };
    serialize_to_builder(node, &serializer);
    sv_free(serializer.escaped.view);
    return serializer.sb.view;
}

ErrorOrInt deserialize_char_entity(XMLDeserializer *deserializer)
{
    ss_reset(&deserializer->ss);
    for (int ch = ss_peek(&deserializer->ss); ch != 0 && ch != ';'; ch = ss_peek(&deserializer->ss)) {
        ss_skip_one(&deserializer->ss);
    }
    StringView code = ss_read_from_mark(&deserializer->ss);
    if (sv_empty(code)) {
        ERROR(Int, XMLError, deserializer->ss.point, "Bad escape- '&#;'");
    }
    if (!ss_expect(&deserializer->ss, ';')) {
        ERROR(Int, XMLError, deserializer->ss.point, "Escape not termininated before end of document");
    }
    IntegerParseResult parse_result = sv_parse_u32(code);
    if (!parse_result.success) {
        ERROR(Int, XMLError, deserializer->ss.point, "Bad escape- '%.*' is not a character entity", SV_ARG(code));
    }
    uint32_t int_value = parse_result.integer.u32;
    char    *int_value_as_char = (char *) (&int_value);
    for (char *p = int_value_as_char; (p - int_value_as_char) < 4 && *p; ++p) {
        sb_append_char(&deserializer->sb, *p);
    }
    RETURN(Int, 0);
}

ErrorOrOptionalStringView deserialize_text(XMLDeserializer *deserializer, StringView close)
{
    deserializer->sb.view.length = 0;
    while (!ss_expect_sv(&deserializer->ss, close) && ss_peek(&deserializer->ss) != 0) {
        int ch = ss_peek(&deserializer->ss);
        switch (ch) {
        case '&': {
            ss_skip_one(&deserializer->ss);
            if (ss_expect_sv(&deserializer->ss, sv_from("amp;"))) {
                sb_append_char(&deserializer->sb, '&');
            } else if (ss_expect_sv(&deserializer->ss, sv_from("apos;"))) {
                sb_append_char(&deserializer->sb, '\'');
            } else if (ss_expect_sv(&deserializer->ss, sv_from("gt;"))) {
                sb_append_char(&deserializer->sb, '>');
            } else if (ss_expect_sv(&deserializer->ss, sv_from("lt;"))) {
                sb_append_char(&deserializer->sb, '<');
            } else if (ss_expect_sv(&deserializer->ss, sv_from("quot;"))) {
                sb_append_char(&deserializer->sb, '\"');
            } else if (ss_expect(&deserializer->ss, '#')) {
                TRY_TO(Int, OptionalStringView, deserialize_char_entity(deserializer));
            } else {
                ERROR(OptionalStringView, XMLError, deserializer->ss.point, "Bad escape");
            }
        } break;
        case '\0':
            ERROR(OptionalStringView, XMLError, deserializer->ss.point, "Unterminated text");
        case '<':
            if (sv_is_whitespace(deserializer->sb.view)) {
                RETURN(OptionalStringView, OptionalStringView_empty());
            }
            // fall through
        case '\'':
        case '\"':
        case '>':
            ERROR(OptionalStringView, XMLError, deserializer->ss.point, "Invalid character in text: '%c'", ch);
        default:
            sb_append_char(&deserializer->sb, ch);
            ss_skip_one(&deserializer->ss);
            break;
        }
    }
    RETURN(OptionalStringView, OptionalStringView_create(deserializer->sb.view));
}

ErrorOrStringView deserialize_tag(XMLDeserializer *deserializer)
{
    ss_reset(&deserializer->ss);
    for (int ch = ss_peek(&deserializer->ss); isalnum(ch) || ch == '.' || ch == '-' || ch == ':' || ch == '_'; ch = ss_peek(&deserializer->ss)) {
        ss_skip_one(&deserializer->ss);
    }
    StringView ret = ss_read_from_mark(&deserializer->ss);
    if (sv_empty(ret)) {
        ERROR(StringView, XMLError, deserializer->ss.point, "Expected tag name");
    }
    trace(CAT_XML, "deserialize_tag(): %.*s", SV_ARG(ret));
    RETURN(StringView, ret);
}

ErrorOrStringView deserialize_attr_value(XMLDeserializer *deserializer)
{
    StringScanner *ss = &deserializer->ss;
    if (!ss_is_one_of(ss, "\"'")) {
        ERROR(StringView, XMLError, deserializer->ss.point, "Expected opening quote");
    }
    StringView close = (StringView) { ss->string.ptr + ss->point, 1 };
    ss_skip_one(ss);
    OptionalStringView ret_maybe = TRY_TO(OptionalStringView, StringView, deserialize_text(deserializer, close));
    if (!ret_maybe.has_value) {
        ERROR(StringView, XMLError, deserializer->ss.point, "No attribute value");
    }
    trace(CAT_XML, "deserialize_attr_value(): %.*s", SV_ARG(ret_maybe.value));
    RETURN(StringView, ret_maybe.value);
}

ErrorOrOptionalXMLNode deserialize_processing_instruction(XMLDeserializer *deserializer, XMLNode parent)
{
    trace(CAT_XML, "Decoding pi");
    ss_skip_whitespace(&deserializer->ss);
    StringView tag = TRY_TO(StringView, OptionalXMLNode, deserialize_tag(deserializer));
    XMLNode    result = xml_processing_instruction(parent, tag);
    ss_skip_whitespace(&deserializer->ss);
    while (ss_peek(&deserializer->ss) != '?') {
        StringView attr = TRY_TO(StringView, OptionalXMLNode, deserialize_tag(deserializer));
        ss_skip_whitespace(&deserializer->ss);
        while (ss_peek(&deserializer->ss) != '=') {
            ERROR(OptionalXMLNode, XMLError, deserializer->ss.point, "Expected =");
        }
        ss_skip_one(&deserializer->ss);
        ss_skip_whitespace(&deserializer->ss);
        StringView value = TRY_TO(StringView, OptionalXMLNode, deserialize_attr_value(deserializer));
        result = xml_set_attribute(result, attr, value);
        ss_skip_whitespace(&deserializer->ss);
    }
    ss_skip_one(&deserializer->ss);
    if (!ss_expect(&deserializer->ss, '>')) {
        ERROR(OptionalXMLNode, XMLError, deserializer->ss.point, "Processing instruction not closed by '?>'");
    }
    trace(CAT_XML, "PI '%.*s' deserialized", SV_ARG(tag));
    RETURN(OptionalXMLNode, OptionalXMLNode_create(result));
}

ErrorOrOptionalXMLNode deserialize_element(XMLDeserializer *deserializer, XMLNode parent)
{
    trace(CAT_XML, "Decoding element");
    StringScanner *ss = &deserializer->ss;
    ss_skip_whitespace(ss);
    StringView tag = TRY_TO(StringView, OptionalXMLNode, deserialize_tag(deserializer));
    XMLNode    result = xml_element(parent, tag);

    ss_skip_whitespace(ss);
    while (!ss_expect_one_of(ss, "/>")) {
        StringView attr = TRY_TO(StringView, OptionalXMLNode, deserialize_tag(deserializer));
        ss_skip_whitespace(ss);
        while (ss_peek(ss) != '=') {
            ERROR(OptionalXMLNode, XMLError, deserializer->ss.point, "Expected =");
        }
        ss_skip_one(ss);
        ss_skip_whitespace(ss);
        StringView value = TRY_TO(StringView, OptionalXMLNode, deserialize_attr_value(deserializer));
        xml_set_attribute(result, attr, value);
        ss_skip_whitespace(ss);
    }
    ss_pushback(ss);
    if (!ss_expect_sv(ss, sv_from("/>"))) {
        assert(ss_expect(ss, '>'));
        while (!ss_expect_sv(ss, sv_from("</"))) {
            TRY(OptionalXMLNode, deserialize_node(deserializer, result));
        }
    }
    if (!ss_expect_sv(ss, tag)) {
        ERROR(OptionalXMLNode, XMLError, deserializer->ss.point, "Expected '</%.s'>", SV_ARG(tag));
    }
    if (!ss_expect(ss, '>')) {
        ERROR(OptionalXMLNode, XMLError, deserializer->ss.point, "Expected >");
    }
    ss_skip_whitespace(&deserializer->ss);
    trace(CAT_XML, "Element '%.*s' [%zu] [%zu] deserialized", SV_ARG(tag), xml_attribute_count(result), xml_child_count(result));
    RETURN(OptionalXMLNode, OptionalXMLNode_create(result));
}

ErrorOrOptionalXMLNode deserialize_node(XMLDeserializer *deserializer, XMLNode parent)
{
    OptionalXMLNode ret = { 0 };

    if (xml_node_type(parent) == XML_TYPE_ELEMENT) {
        StringView debug = xml_debug(parent);
        trace(CAT_XML, "deserialize_node(%zu, %.*s)", deserializer->ss.point, SV_ARG(debug));
        sv_free(debug);
    } else {
        trace(CAT_XML, "deserialize_node(%zu, [%zu] %.*s)", deserializer->ss.point, parent.index, SV_ARG(xml_to_string(parent)));
    }
    // ss_skip_whitespace(&deserializer->ss);
    switch (ss_peek(&deserializer->ss)) {
    case 0:
        ERROR(OptionalXMLNode, XMLError, deserializer->ss.point, "Expected node");
    case '<': {
        trace(CAT_XML, "Found '<'");
        ss_skip_one(&deserializer->ss);
        if (ss_peek(&deserializer->ss) == '?') {
            trace(CAT_XML, "Found '<?'");
            ss_skip_one(&deserializer->ss);
            ret = TRY(OptionalXMLNode, deserialize_processing_instruction(deserializer, parent));
            break;
        }
        ret = TRY(OptionalXMLNode, deserialize_element(deserializer, parent));
        break;
    }
    default: {
        trace(CAT_XML, "Decoding text");
        OptionalStringView txt_maybe = TRY_TO(OptionalStringView, OptionalXMLNode, deserialize_text(deserializer, sv_from("</")));
        if (txt_maybe.has_value) {
            ss_pushback(&deserializer->ss);
            ss_pushback(&deserializer->ss);
            ret = OptionalXMLNode_create(xml_text(parent, txt_maybe.value));
        } else {
            trace(CAT_XML, "Whitespace preceding element");
        }
        break;
    }
    }
    RETURN(OptionalXMLNode, ret);
}

ErrorOrXMLNode xml_deserialize(StringView xml)
{
    XMLDeserializer deserializer = { .sb = sb_create(), .ss = ss_create(xml) };

    XMLNode doc = xml_document();
    ss_skip_whitespace(&deserializer.ss);
    while (ss_peek(&deserializer.ss)) {
        OptionalXMLNode node = TRY_TO(OptionalXMLNode, XMLNode, deserialize_node(&deserializer, doc));
        if (!node.has_value) {
            trace(CAT_XML, "Deserialized NULL");
            continue;
        }
        trace(CAT_XML, "Deserialized node '%.*s'", SV_ARG(xml_to_string(node.value)));
    }
    sv_free(deserializer.sb.view);
    RETURN(XMLNode, doc);
}

#ifdef XML_FORMAT

#include <io.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        return -1;
    }
    log_init();
    StringView sv = MUST(StringView, read_file_by_name(sv_from(argv[1])));
    XMLNode    doc = MUST(XMLNode, xml_deserialize(sv));
    StringView xml = xml_serialize(doc);
    printf("%.*s\n", SV_ARG(xml));
    xml_free(doc);
    return 0;
}

#endif
