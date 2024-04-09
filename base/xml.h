/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BASE_XML_H__
#define __BASE_XML_H__

#include <sv.h>

#define XMLTYPES(S)           \
    S(ELEMENT)                \
    S(TEXT)                   \
    S(ATTRIBUTE)              \
    S(PROCESSING_INSTRUCTION) \
    S(DOCUMENT)

typedef enum {
#undef XMLTYPE
#define XMLTYPE(T) XML_TYPE_##T,
    XMLTYPES(XMLTYPE)
#undef XMLTYPE
} XMLType;

typedef struct {
    XMLType type;
    size_t  parent;
    size_t  index;
    union {
        // Make sure that element and pi tag and attributes field line up.
        // Make sure that element and document children field line up.
        struct {
            StringView tag;
            Sizes      attributes;
            Sizes      children;
        } element;
        struct {
            StringView tag;
            Sizes      attributes;
        } pi;
        // StringBuilder and StringView are the same size!
        struct {
            StringBuilder sb;
            Sizes         processing_instructions;
            Sizes         children;
        } document;
        StringView text;
        struct {
            StringView tag;
            StringView text;
        } attribute;
    };
} XMLNodeImpl;

DA_WITH_NAME(XMLNodeImpl, XMLNodeImpls);

typedef struct {
    XMLNodeImpls *registry;
    size_t        index;
} XMLNode;

DA_WITH_NAME(XMLNode, XMLNodes);
OPTIONAL(XMLNode)
ERROR_OR(XMLNode)
ERROR_OR(OptionalXMLNode)

extern char const        *XMLType_name(XMLType type);
extern StringView         xml_to_string(XMLNode node);
extern StringView         xml_debug(XMLNode node);
extern XMLNode            xml_document();
extern XMLNode            xml_document_fragment();
extern XMLNode            xml_processing_instruction(XMLNode parent, StringView tag);
extern XMLNode            xml_element(XMLNode parent, StringView tag);
extern XMLNode            xml_text(XMLNode parent, StringView text);
extern XMLNode            xml_text_element(XMLNode parent, StringView tag, StringView text);
extern void               xml_free(XMLNode node);
extern XMLNode            xml_set_attribute(XMLNode node, StringView attr, StringView text);
extern XMLNode            xml_document_of(XMLNode node);
extern XMLType            xml_node_type(XMLNode node);
extern size_t             xml_child_count(XMLNode node);
extern XMLNode            xml_child(XMLNode node, size_t ix);
extern XMLNodes           xml_children_by_tag(XMLNode node, StringView tag);
extern OptionalXMLNode    xml_first_child_by_tag(XMLNode node, StringView tag);
extern size_t             xml_attribute_count(XMLNode node);
extern XMLNode            xml_attribute(XMLNode node, size_t ix);
extern OptionalXMLNode    xml_attribute_by_tag(XMLNode node, StringView tag);
extern StringView         xml_tag(XMLNode node);
extern OptionalStringView xml_text_of(XMLNode node);
extern StringView         xml_serialize(XMLNode node);
extern ErrorOrXMLNode     xml_deserialize(StringView xml_text);

#endif /* __BASE_XML_H__ */
