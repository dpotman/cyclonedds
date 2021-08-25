/****************************************************************

  Generated by Eclipse Cyclone DDS IDL to C Translator
  File name: typemap.c
  Source: typemap.idl
  Cyclone DDS: V0.8.0

*****************************************************************/
#include "dds/ddsi/ddsi_xt_typemap.h"

static const uint32_t DDS_XTypes_TypeMapping_ops [] =
{
  /* TypeMapping */
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_TypeMapping, identifier_object_pair_minimal), sizeof (DDS_XTypes_TypeIdentifierTypeObjectPair), (4u << 16u) + 13u /* TypeIdentifierTypeObjectPair */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_TypeMapping, identifier_object_pair_complete), sizeof (DDS_XTypes_TypeIdentifierTypeObjectPair), (4u << 16u) + 9u /* TypeIdentifierTypeObjectPair */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_TypeMapping, identifier_complete_minimal), sizeof (DDS_XTypes_TypeIdentifierPair), (4u << 16u) + 875u /* TypeIdentifierPair */,
  DDS_OP_RTS,

  /* TypeIdentifierTypeObjectPair */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_TypeIdentifierTypeObjectPair, type_identifier), (3u << 16u) + 7u /* TypeIdentifier */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_TypeIdentifierTypeObjectPair, type_object), (3u << 16u) + 162u /* TypeObject */,
  DDS_OP_RTS,

  /* TypeIdentifier */
  DDS_OP_ADR | DDS_OP_TYPE_UNI | DDS_OP_SUBTYPE_1BY | DDS_OP_FLAG_DEF, offsetof (DDS_XTypes_TypeIdentifier, _d), 13u, (50u << 16u) + 4u,
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 47 /* StringSTypeDefn */, 112, offsetof (DDS_XTypes_TypeIdentifier, _u.string_sdefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 44 /* StringSTypeDefn */, 114, offsetof (DDS_XTypes_TypeIdentifier, _u.string_sdefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 44 /* StringLTypeDefn */, 113, offsetof (DDS_XTypes_TypeIdentifier, _u.string_ldefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 41 /* StringLTypeDefn */, 115, offsetof (DDS_XTypes_TypeIdentifier, _u.string_ldefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 41 /* PlainSequenceSElemDefn */, 128, offsetof (DDS_XTypes_TypeIdentifier, _u.seq_sdefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 53 /* PlainSequenceLElemDefn */, 129, offsetof (DDS_XTypes_TypeIdentifier, _u.seq_ldefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 60 /* PlainArraySElemDefn */, 144, offsetof (DDS_XTypes_TypeIdentifier, _u.array_sdefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 67 /* PlainArrayLElemDefn */, 145, offsetof (DDS_XTypes_TypeIdentifier, _u.array_ldefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 74 /* PlainMapSTypeDefn */, 160, offsetof (DDS_XTypes_TypeIdentifier, _u.map_sdefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 87 /* PlainMapLTypeDefn */, 161, offsetof (DDS_XTypes_TypeIdentifier, _u.map_ldefn),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 100 /* StronglyConnectedComponentId */, 176, offsetof (DDS_XTypes_TypeIdentifier, _u.sc_component_id),
  DDS_OP_JEQ | DDS_OP_TYPE_ARR | 9, 242, offsetof (DDS_XTypes_TypeIdentifier, _u.equivalence_hash),
  DDS_OP_JEQ | DDS_OP_TYPE_ARR | 6, 241, offsetof (DDS_XTypes_TypeIdentifier, _u.equivalence_hash),
  DDS_OP_RTS,
  DDS_OP_RTS,
  DDS_OP_RTS,
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, 0u, 14u,
  DDS_OP_RTS,
  DDS_OP_RTS,

  /* StringSTypeDefn */
  DDS_OP_ADR | DDS_OP_TYPE_1BY, offsetof (DDS_XTypes_StringSTypeDefn, bound),
  DDS_OP_RTS,

  /* StringLTypeDefn */
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (DDS_XTypes_StringLTypeDefn, bound),
  DDS_OP_RTS,

  /* PlainSequenceSElemDefn */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_PlainSequenceSElemDefn, header), (3u << 16u) + 10u /* PlainCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_1BY, offsetof (DDS_XTypes_PlainSequenceSElemDefn, bound),
  DDS_OP_ADR | DDS_OP_TYPE_EXT | DDS_OP_FLAG_EXT, offsetof (DDS_XTypes_PlainSequenceSElemDefn, element_identifier), (4u << 16u) + 65474u /* TypeIdentifier */, sizeof (DDS_XTypes_TypeIdentifier),
  DDS_OP_RTS,

  /* PlainCollectionHeader */
  DDS_OP_ADR | DDS_OP_TYPE_1BY, offsetof (DDS_XTypes_PlainCollectionHeader, equiv_kind),
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_PlainCollectionHeader, element_flags),
  DDS_OP_RTS,

  /* PlainSequenceLElemDefn */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_PlainSequenceLElemDefn, header), (3u << 16u) + 65531u /* PlainCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (DDS_XTypes_PlainSequenceLElemDefn, bound),
  DDS_OP_ADR | DDS_OP_TYPE_EXT | DDS_OP_FLAG_EXT, offsetof (DDS_XTypes_PlainSequenceLElemDefn, element_identifier), (4u << 16u) + 65459u /* TypeIdentifier */, sizeof (DDS_XTypes_TypeIdentifier),
  DDS_OP_RTS,

  /* PlainArraySElemDefn */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_PlainArraySElemDefn, header), (3u << 16u) + 65521u /* PlainCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_1BY, offsetof (DDS_XTypes_PlainArraySElemDefn, array_bound_seq),
  DDS_OP_ADR | DDS_OP_TYPE_EXT | DDS_OP_FLAG_EXT, offsetof (DDS_XTypes_PlainArraySElemDefn, element_identifier), (4u << 16u) + 65449u /* TypeIdentifier */, sizeof (DDS_XTypes_TypeIdentifier),
  DDS_OP_RTS,

  /* PlainArrayLElemDefn */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_PlainArrayLElemDefn, header), (3u << 16u) + 65511u /* PlainCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_4BY, offsetof (DDS_XTypes_PlainArrayLElemDefn, array_bound_seq),
  DDS_OP_ADR | DDS_OP_TYPE_EXT | DDS_OP_FLAG_EXT, offsetof (DDS_XTypes_PlainArrayLElemDefn, element_identifier), (4u << 16u) + 65439u /* TypeIdentifier */, sizeof (DDS_XTypes_TypeIdentifier),
  DDS_OP_RTS,

  /* PlainMapSTypeDefn */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_PlainMapSTypeDefn, header), (3u << 16u) + 65501u /* PlainCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_1BY, offsetof (DDS_XTypes_PlainMapSTypeDefn, bound),
  DDS_OP_ADR | DDS_OP_TYPE_EXT | DDS_OP_FLAG_EXT, offsetof (DDS_XTypes_PlainMapSTypeDefn, element_identifier), (4u << 16u) + 65429u /* TypeIdentifier */, sizeof (DDS_XTypes_TypeIdentifier),
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_PlainMapSTypeDefn, key_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT | DDS_OP_FLAG_EXT, offsetof (DDS_XTypes_PlainMapSTypeDefn, key_identifier), (4u << 16u) + 65423u /* TypeIdentifier */, sizeof (DDS_XTypes_TypeIdentifier),
  DDS_OP_RTS,

  /* PlainMapLTypeDefn */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_PlainMapLTypeDefn, header), (3u << 16u) + 65485u /* PlainCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (DDS_XTypes_PlainMapLTypeDefn, bound),
  DDS_OP_ADR | DDS_OP_TYPE_EXT | DDS_OP_FLAG_EXT, offsetof (DDS_XTypes_PlainMapLTypeDefn, element_identifier), (4u << 16u) + 65413u /* TypeIdentifier */, sizeof (DDS_XTypes_TypeIdentifier),
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_PlainMapLTypeDefn, key_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT | DDS_OP_FLAG_EXT, offsetof (DDS_XTypes_PlainMapLTypeDefn, key_identifier), (4u << 16u) + 65407u /* TypeIdentifier */, sizeof (DDS_XTypes_TypeIdentifier),
  DDS_OP_RTS,

  /* StronglyConnectedComponentId */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_StronglyConnectedComponentId, sc_component_id), (3u << 16u) + 8u /* TypeObjectHashId */,
  DDS_OP_ADR | DDS_OP_TYPE_4BY | DDS_OP_FLAG_SGN, offsetof (DDS_XTypes_StronglyConnectedComponentId, scc_length),
  DDS_OP_ADR | DDS_OP_TYPE_4BY | DDS_OP_FLAG_SGN, offsetof (DDS_XTypes_StronglyConnectedComponentId, scc_index),
  DDS_OP_RTS,

  /* TypeObjectHashId */
  DDS_OP_ADR | DDS_OP_TYPE_UNI | DDS_OP_SUBTYPE_1BY, offsetof (DDS_XTypes_TypeObjectHashId, _d), 2u, (14u << 16u) + 4u,
  DDS_OP_JEQ | DDS_OP_TYPE_ARR | 6, 242, offsetof (DDS_XTypes_TypeObjectHashId, _u.hash),
  DDS_OP_JEQ | DDS_OP_TYPE_ARR | 3, 241, offsetof (DDS_XTypes_TypeObjectHashId, _u.hash),
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, 0u, 14u,
  DDS_OP_RTS,
  DDS_OP_RTS,

  /* TypeObject */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_UNI | DDS_OP_SUBTYPE_1BY, offsetof (DDS_XTypes_TypeObject, _d), 2u, (10u << 16u) + 4u,
  DDS_OP_JEQ | DDS_OP_TYPE_UNI | 7 /* CompleteTypeObject */, 242, offsetof (DDS_XTypes_TypeObject, _u.complete),
  DDS_OP_JEQ | DDS_OP_TYPE_UNI | 477 /* MinimalTypeObject */, 241, offsetof (DDS_XTypes_TypeObject, _u.minimal),
  DDS_OP_RTS,

  /* CompleteTypeObject */
  DDS_OP_ADR | DDS_OP_TYPE_UNI | DDS_OP_SUBTYPE_1BY | DDS_OP_FLAG_DEF, offsetof (DDS_XTypes_CompleteTypeObject, _d), 10u, (37u << 16u) + 4u,
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 34 /* CompleteAliasType */, 48, offsetof (DDS_XTypes_CompleteTypeObject, _u.alias_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 168 /* CompleteAnnotationType */, 80, offsetof (DDS_XTypes_CompleteTypeObject, _u.annotation_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 197 /* CompleteStructType */, 81, offsetof (DDS_XTypes_CompleteTypeObject, _u.struct_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 239 /* CompleteUnionType */, 82, offsetof (DDS_XTypes_CompleteTypeObject, _u.union_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 290 /* CompleteBitsetType */, 83, offsetof (DDS_XTypes_CompleteTypeObject, _u.bitset_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 320 /* CompleteSequenceType */, 96, offsetof (DDS_XTypes_CompleteTypeObject, _u.sequence_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 359 /* CompleteArrayType */, 97, offsetof (DDS_XTypes_CompleteTypeObject, _u.array_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 377 /* CompleteMapType */, 98, offsetof (DDS_XTypes_CompleteTypeObject, _u.map_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 386 /* CompleteEnumeratedType */, 64, offsetof (DDS_XTypes_CompleteTypeObject, _u.enumerated_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 418 /* CompleteBitmaskType */, 65, offsetof (DDS_XTypes_CompleteTypeObject, _u.bitmask_type),
  DDS_OP_RTS,
  DDS_OP_RTS,
  DDS_OP_RTS,
  DDS_OP_RTS,

  /* CompleteAliasType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteAliasType, alias_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteAliasType, header), (3u << 16u) + 7u /* CompleteAliasHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteAliasType, body), (3u << 16u) + 102u /* CompleteAliasBody */,
  DDS_OP_RTS,

  /* CompleteAliasHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteAliasHeader, detail), (3u << 16u) + 4u /* CompleteTypeDetail */,
  DDS_OP_RTS,

  /* CompleteTypeDetail */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteTypeDetail, ann_builtin), (3u << 16u) + 11u /* AppliedBuiltinTypeAnnotations */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteTypeDetail, ann_custom), sizeof (DDS_XTypes_AppliedAnnotation), (4u << 16u) + 22u /* AppliedAnnotation */,
  DDS_OP_ADR | DDS_OP_TYPE_BST, offsetof (DDS_XTypes_CompleteTypeDetail, type_name), 257u,
  DDS_OP_RTS,

  /* AppliedBuiltinTypeAnnotations */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_AppliedBuiltinTypeAnnotations, verbatim), (3u << 16u) + 4u /* AppliedVerbatimAnnotation */,
  DDS_OP_RTS,

  /* AppliedVerbatimAnnotation */
  DDS_OP_ADR | DDS_OP_TYPE_BST, offsetof (DDS_XTypes_AppliedVerbatimAnnotation, placement), 33u,
  DDS_OP_ADR | DDS_OP_TYPE_BST, offsetof (DDS_XTypes_AppliedVerbatimAnnotation, language), 33u,
  DDS_OP_ADR | DDS_OP_TYPE_STR, offsetof (DDS_XTypes_AppliedVerbatimAnnotation, text),
  DDS_OP_RTS,

  /* AppliedAnnotation */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_AppliedAnnotation, annotation_typeid), (3u << 16u) + 65288u /* TypeIdentifier */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_AppliedAnnotation, param_seq), sizeof (DDS_XTypes_AppliedAnnotationParameter), (4u << 16u) + 5u /* AppliedAnnotationParameter */,
  DDS_OP_RTS,

  /* AppliedAnnotationParameter */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, offsetof (DDS_XTypes_AppliedAnnotationParameter, paramname_hash), 4u,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_AppliedAnnotationParameter, value), (3u << 16u) + 4u /* AnnotationParameterValue */,
  DDS_OP_RTS,

  /* AnnotationParameterValue */
  DDS_OP_ADR | DDS_OP_TYPE_UNI | DDS_OP_SUBTYPE_1BY | DDS_OP_FLAG_DEF, offsetof (DDS_XTypes_AnnotationParameterValue, _d), 13u, (50u << 16u) + 4u,
  DDS_OP_JEQ | DDS_OP_TYPE_1BY | 0, 1, offsetof (DDS_XTypes_AnnotationParameterValue, _u.boolean_value),
  DDS_OP_JEQ | DDS_OP_TYPE_1BY | 0, 2, offsetof (DDS_XTypes_AnnotationParameterValue, _u.byte_value),
  DDS_OP_JEQ | DDS_OP_TYPE_2BY | 0, 3, offsetof (DDS_XTypes_AnnotationParameterValue, _u.int16_value),
  DDS_OP_JEQ | DDS_OP_TYPE_2BY | 0, 6, offsetof (DDS_XTypes_AnnotationParameterValue, _u.uint_16_value),
  DDS_OP_JEQ | DDS_OP_TYPE_4BY | 0, 4, offsetof (DDS_XTypes_AnnotationParameterValue, _u.int32_value),
  DDS_OP_JEQ | DDS_OP_TYPE_4BY | 0, 7, offsetof (DDS_XTypes_AnnotationParameterValue, _u.uint32_value),
  DDS_OP_JEQ | DDS_OP_TYPE_8BY | 0, 5, offsetof (DDS_XTypes_AnnotationParameterValue, _u.int64_value),
  DDS_OP_JEQ | DDS_OP_TYPE_8BY | 0, 8, offsetof (DDS_XTypes_AnnotationParameterValue, _u.uint64_value),
  DDS_OP_JEQ | DDS_OP_TYPE_4BY | 0, 9, offsetof (DDS_XTypes_AnnotationParameterValue, _u.float32_value),
  DDS_OP_JEQ | DDS_OP_TYPE_8BY | 0, 10, offsetof (DDS_XTypes_AnnotationParameterValue, _u.float64_value),
  DDS_OP_JEQ | DDS_OP_TYPE_1BY | 0, 16, offsetof (DDS_XTypes_AnnotationParameterValue, _u.char_value),
  DDS_OP_JEQ | DDS_OP_TYPE_4BY | 0, 64, offsetof (DDS_XTypes_AnnotationParameterValue, _u.enumerated_value),
  DDS_OP_JEQ | DDS_OP_TYPE_BST | 6, 32, offsetof (DDS_XTypes_AnnotationParameterValue, _u.string8_value),
  DDS_OP_RTS,
  DDS_OP_RTS,
  DDS_OP_RTS,
  DDS_OP_ADR | DDS_OP_TYPE_BST, offsetof (DDS_XTypes_AnnotationParameterValue, _u.string8_value), 129u,
  DDS_OP_RTS,
  DDS_OP_RTS,

  /* CompleteAliasBody */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteAliasBody, common), (3u << 16u) + 11u /* CommonAliasBody */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteAliasBody, ann_builtin), (3u << 16u) + 14u /* AppliedBuiltinMemberAnnotations */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteAliasBody, ann_custom), sizeof (DDS_XTypes_AppliedAnnotation), (4u << 16u) + 65461u /* AppliedAnnotation */,
  DDS_OP_RTS,

  /* CommonAliasBody */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonAliasBody, related_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CommonAliasBody, related_type), (3u << 16u) + 65207u /* TypeIdentifier */,
  DDS_OP_RTS,

  /* AppliedBuiltinMemberAnnotations */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_STR, offsetof (DDS_XTypes_AppliedBuiltinMemberAnnotations, unit),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_AppliedBuiltinMemberAnnotations, min), (3u << 16u) + 65464u /* AnnotationParameterValue */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_AppliedBuiltinMemberAnnotations, max), (3u << 16u) + 65461u /* AnnotationParameterValue */,
  DDS_OP_ADR | DDS_OP_TYPE_STR, offsetof (DDS_XTypes_AppliedBuiltinMemberAnnotations, hash_id),
  DDS_OP_RTS,

  /* CompleteAnnotationType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteAnnotationType, annotation_flag),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteAnnotationType, header), (3u << 16u) + 8u /* CompleteAnnotationHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteAnnotationType, member_seq), sizeof (DDS_XTypes_CompleteAnnotationParameter), (4u << 16u) + 10u /* CompleteAnnotationParameter */,
  DDS_OP_RTS,

  /* CompleteAnnotationHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_BST, offsetof (DDS_XTypes_CompleteAnnotationHeader, annotation_name), 257u,
  DDS_OP_RTS,

  /* CompleteAnnotationParameter */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteAnnotationParameter, common), (3u << 16u) + 10u /* CommonAnnotationParameter */,
  DDS_OP_ADR | DDS_OP_TYPE_BST, offsetof (DDS_XTypes_CompleteAnnotationParameter, name), 257u,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteAnnotationParameter, default_value), (3u << 16u) + 65433u /* AnnotationParameterValue */,
  DDS_OP_RTS,

  /* CommonAnnotationParameter */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonAnnotationParameter, member_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CommonAnnotationParameter, member_type_id), (3u << 16u) + 65163u /* TypeIdentifier */,
  DDS_OP_RTS,

  /* CompleteStructType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteStructType, struct_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteStructType, header), (3u << 16u) + 8u /* CompleteStructHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteStructType, member_seq), sizeof (DDS_XTypes_CompleteStructMember), (4u << 16u) + 13u /* CompleteStructMember */,
  DDS_OP_RTS,

  /* CompleteStructHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteStructHeader, base_type), (3u << 16u) + 65148u /* TypeIdentifier */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteStructHeader, detail), (3u << 16u) + 65367u /* CompleteTypeDetail */,
  DDS_OP_RTS,

  /* CompleteStructMember */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteStructMember, common), (3u << 16u) + 7u /* CommonStructMember */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteStructMember, detail), (3u << 16u) + 12u /* CompleteMemberDetail */,
  DDS_OP_RTS,

  /* CommonStructMember */
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (DDS_XTypes_CommonStructMember, member_id),
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonStructMember, member_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CommonStructMember, member_type_id), (3u << 16u) + 65129u /* TypeIdentifier */,
  DDS_OP_RTS,

  /* CompleteMemberDetail */
  DDS_OP_ADR | DDS_OP_TYPE_BST, offsetof (DDS_XTypes_CompleteMemberDetail, name), 257u,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteMemberDetail, ann_builtin), (3u << 16u) + 65455u /* AppliedBuiltinMemberAnnotations */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteMemberDetail, ann_custom), sizeof (DDS_XTypes_AppliedAnnotation), (4u << 16u) + 65366u /* AppliedAnnotation */,
  DDS_OP_RTS,

  /* CompleteUnionType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteUnionType, union_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteUnionType, header), (3u << 16u) + 11u /* CompleteUnionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteUnionType, discriminator), (3u << 16u) + 13u /* CompleteDiscriminatorMember */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteUnionType, member_seq), sizeof (DDS_XTypes_CompleteUnionMember), (4u << 16u) + 28u /* CompleteUnionMember */,
  DDS_OP_RTS,

  /* CompleteUnionHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteUnionHeader, detail), (3u << 16u) + 65322u /* CompleteTypeDetail */,
  DDS_OP_RTS,

  /* CompleteDiscriminatorMember */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteDiscriminatorMember, common), (3u << 16u) + 11u /* CommonDiscriminatorMember */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteDiscriminatorMember, ann_builtin), (3u << 16u) + 65325u /* AppliedBuiltinTypeAnnotations */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteDiscriminatorMember, ann_custom), sizeof (DDS_XTypes_AppliedAnnotation), (4u << 16u) + 65336u /* AppliedAnnotation */,
  DDS_OP_RTS,

  /* CommonDiscriminatorMember */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonDiscriminatorMember, member_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CommonDiscriminatorMember, type_id), (3u << 16u) + 65082u /* TypeIdentifier */,
  DDS_OP_RTS,

  /* CompleteUnionMember */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteUnionMember, common), (3u << 16u) + 7u /* CommonUnionMember */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteUnionMember, detail), (3u << 16u) + 65485u /* CompleteMemberDetail */,
  DDS_OP_RTS,

  /* CommonUnionMember */
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (DDS_XTypes_CommonUnionMember, member_id),
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonUnionMember, member_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CommonUnionMember, type_id), (3u << 16u) + 65066u /* TypeIdentifier */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_4BY | DDS_OP_FLAG_SGN, offsetof (DDS_XTypes_CommonUnionMember, label_seq),
  DDS_OP_RTS,

  /* CompleteBitsetType */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteBitsetType, bitset_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteBitsetType, header), (3u << 16u) + 8u /* CompleteBitsetHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteBitsetType, field_seq), sizeof (DDS_XTypes_CompleteBitfield), (4u << 16u) + 10u /* CompleteBitfield */,
  DDS_OP_RTS,

  /* CompleteBitsetHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteBitsetHeader, detail), (3u << 16u) + 65270u /* CompleteTypeDetail */,
  DDS_OP_RTS,

  /* CompleteBitfield */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteBitfield, common), (3u << 16u) + 7u /* CommonBitfield */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteBitfield, detail), (3u << 16u) + 65451u /* CompleteMemberDetail */,
  DDS_OP_RTS,

  /* CommonBitfield */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonBitfield, position),
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonBitfield, flags),
  DDS_OP_ADR | DDS_OP_TYPE_1BY, offsetof (DDS_XTypes_CommonBitfield, bitcount),
  DDS_OP_ADR | DDS_OP_TYPE_1BY, offsetof (DDS_XTypes_CommonBitfield, holder_type),
  DDS_OP_RTS,

  /* CompleteSequenceType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteSequenceType, collection_flag),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteSequenceType, header), (3u << 16u) + 7u /* CompleteCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteSequenceType, element), (3u << 16u) + 15u /* CompleteCollectionElement */,
  DDS_OP_RTS,

  /* CompleteCollectionHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteCollectionHeader, common), (3u << 16u) + 7u /* CommonCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteCollectionHeader, detail), (3u << 16u) + 65236u /* CompleteTypeDetail */,
  DDS_OP_RTS,

  /* CommonCollectionHeader */
  DDS_OP_ADR | DDS_OP_TYPE_4BY, offsetof (DDS_XTypes_CommonCollectionHeader, bound),
  DDS_OP_RTS,

  /* CompleteCollectionElement */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteCollectionElement, common), (3u << 16u) + 7u /* CommonCollectionElement */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteCollectionElement, detail), (3u << 16u) + 10u /* CompleteElementDetail */,
  DDS_OP_RTS,

  /* CommonCollectionElement */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonCollectionElement, element_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CommonCollectionElement, type), (3u << 16u) + 64997u /* TypeIdentifier */,
  DDS_OP_RTS,

  /* CompleteElementDetail */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteElementDetail, ann_builtin), (3u << 16u) + 65326u /* AppliedBuiltinMemberAnnotations */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteElementDetail, ann_custom), sizeof (DDS_XTypes_AppliedAnnotation), (4u << 16u) + 65237u /* AppliedAnnotation */,
  DDS_OP_RTS,

  /* CompleteArrayType */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteArrayType, collection_flag),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteArrayType, header), (3u << 16u) + 7u /* CompleteArrayHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteArrayType, element), (3u << 16u) + 65508u /* CompleteCollectionElement */,
  DDS_OP_RTS,

  /* CompleteArrayHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteArrayHeader, common), (3u << 16u) + 7u /* CommonArrayHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteArrayHeader, detail), (3u << 16u) + 65193u /* CompleteTypeDetail */,
  DDS_OP_RTS,

  /* CommonArrayHeader */
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_4BY, offsetof (DDS_XTypes_CommonArrayHeader, bound_seq),
  DDS_OP_RTS,

  /* CompleteMapType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteMapType, collection_flag),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteMapType, header), (3u << 16u) + 65480u /* CompleteCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteMapType, key), (3u << 16u) + 65488u /* CompleteCollectionElement */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteMapType, element), (3u << 16u) + 65485u /* CompleteCollectionElement */,
  DDS_OP_RTS,

  /* CompleteEnumeratedType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteEnumeratedType, enum_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteEnumeratedType, header), (3u << 16u) + 8u /* CompleteEnumeratedHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteEnumeratedType, literal_seq), sizeof (DDS_XTypes_CompleteEnumeratedLiteral), (4u << 16u) + 16u /* CompleteEnumeratedLiteral */,
  DDS_OP_RTS,

  /* CompleteEnumeratedHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteEnumeratedHeader, common), (3u << 16u) + 7u /* CommonEnumeratedHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteEnumeratedHeader, detail), (3u << 16u) + 65160u /* CompleteTypeDetail */,
  DDS_OP_RTS,

  /* CommonEnumeratedHeader */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonEnumeratedHeader, bit_bound),
  DDS_OP_RTS,

  /* CompleteEnumeratedLiteral */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteEnumeratedLiteral, common), (3u << 16u) + 7u /* CommonEnumeratedLiteral */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteEnumeratedLiteral, detail), (3u << 16u) + 65338u /* CompleteMemberDetail */,
  DDS_OP_RTS,

  /* CommonEnumeratedLiteral */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_4BY | DDS_OP_FLAG_SGN, offsetof (DDS_XTypes_CommonEnumeratedLiteral, value),
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonEnumeratedLiteral, flags),
  DDS_OP_RTS,

  /* CompleteBitmaskType */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CompleteBitmaskType, bitmask_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteBitmaskType, header), (3u << 16u) + 65508u /* CompleteEnumeratedHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_CompleteBitmaskType, flag_seq), sizeof (DDS_XTypes_CompleteBitflag), (4u << 16u) + 5u /* CompleteBitflag */,
  DDS_OP_RTS,

  /* CompleteBitflag */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteBitflag, common), (3u << 16u) + 7u /* CommonBitflag */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_CompleteBitflag, detail), (3u << 16u) + 65313u /* CompleteMemberDetail */,
  DDS_OP_RTS,

  /* CommonBitflag */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonBitflag, position),
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_CommonBitflag, flags),
  DDS_OP_RTS,

  /* MinimalTypeObject */
  DDS_OP_ADR | DDS_OP_TYPE_UNI | DDS_OP_SUBTYPE_1BY | DDS_OP_FLAG_DEF, offsetof (DDS_XTypes_MinimalTypeObject, _d), 10u, (37u << 16u) + 4u,
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 34 /* MinimalAliasType */, 48, offsetof (DDS_XTypes_MinimalTypeObject, _u.alias_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 42 /* MinimalAnnotationType */, 80, offsetof (DDS_XTypes_MinimalTypeObject, _u.annotation_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 57 /* MinimalStructType */, 81, offsetof (DDS_XTypes_MinimalTypeObject, _u.struct_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 81 /* MinimalUnionType */, 82, offsetof (DDS_XTypes_MinimalTypeObject, _u.union_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 101 /* MinimalBitsetType */, 83, offsetof (DDS_XTypes_MinimalTypeObject, _u.bitset_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 114 /* MinimalSequenceType */, 96, offsetof (DDS_XTypes_MinimalTypeObject, _u.sequence_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 130 /* MinimalArrayType */, 97, offsetof (DDS_XTypes_MinimalTypeObject, _u.array_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 141 /* MinimalMapType */, 98, offsetof (DDS_XTypes_MinimalTypeObject, _u.map_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 150 /* MinimalEnumeratedType */, 64, offsetof (DDS_XTypes_MinimalTypeObject, _u.enumerated_type),
  DDS_OP_JEQ | DDS_OP_TYPE_STU | 170 /* MinimalBitmaskType */, 65, offsetof (DDS_XTypes_MinimalTypeObject, _u.bitmask_type),
  DDS_OP_RTS,
  DDS_OP_RTS,
  DDS_OP_RTS,
  DDS_OP_RTS,

  /* MinimalAliasType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalAliasType, alias_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalAliasType, body), (3u << 16u) + 4u /* MinimalAliasBody */,
  DDS_OP_RTS,

  /* MinimalAliasBody */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalAliasBody, common), (3u << 16u) + 65175u /* CommonAliasBody */,
  DDS_OP_RTS,

  /* MinimalAnnotationType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalAnnotationType, annotation_flag),
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_MinimalAnnotationType, member_seq), sizeof (DDS_XTypes_MinimalAnnotationParameter), (4u << 16u) + 5u /* MinimalAnnotationParameter */,
  DDS_OP_RTS,

  /* MinimalAnnotationParameter */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalAnnotationParameter, common), (3u << 16u) + 65207u /* CommonAnnotationParameter */,
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, offsetof (DDS_XTypes_MinimalAnnotationParameter, name_hash), 4u,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalAnnotationParameter, default_value), (3u << 16u) + 65094u /* AnnotationParameterValue */,
  DDS_OP_RTS,

  /* MinimalStructType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalStructType, struct_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalStructType, header), (3u << 16u) + 8u /* MinimalStructHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_MinimalStructType, member_seq), sizeof (DDS_XTypes_MinimalStructMember), (4u << 16u) + 10u /* MinimalStructMember */,
  DDS_OP_RTS,

  /* MinimalStructHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalStructHeader, base_type), (3u << 16u) + 64815u /* TypeIdentifier */,
  DDS_OP_RTS,

  /* MinimalStructMember */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalStructMember, common), (3u << 16u) + 65213u /* CommonStructMember */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalStructMember, detail), (3u << 16u) + 4u /* MinimalMemberDetail */,
  DDS_OP_RTS,

  /* MinimalMemberDetail */
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, offsetof (DDS_XTypes_MinimalMemberDetail, name_hash), 4u,
  DDS_OP_RTS,

  /* MinimalUnionType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalUnionType, union_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalUnionType, discriminator), (3u << 16u) + 8u /* MinimalDiscriminatorMember */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_MinimalUnionType, member_seq), sizeof (DDS_XTypes_MinimalUnionMember), (4u << 16u) + 10u /* MinimalUnionMember */,
  DDS_OP_RTS,

  /* MinimalDiscriminatorMember */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalDiscriminatorMember, common), (3u << 16u) + 65240u /* CommonDiscriminatorMember */,
  DDS_OP_RTS,

  /* MinimalUnionMember */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalUnionMember, common), (3u << 16u) + 65249u /* CommonUnionMember */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalUnionMember, detail), (3u << 16u) + 65513u /* MinimalMemberDetail */,
  DDS_OP_RTS,

  /* MinimalBitsetType */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalBitsetType, bitset_flags),
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_MinimalBitsetType, field_seq), sizeof (DDS_XTypes_MinimalBitfield), (4u << 16u) + 5u /* MinimalBitfield */,
  DDS_OP_RTS,

  /* MinimalBitfield */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalBitfield, common), (3u << 16u) + 65267u /* CommonBitfield */,
  DDS_OP_ADR | DDS_OP_TYPE_ARR | DDS_OP_SUBTYPE_1BY, offsetof (DDS_XTypes_MinimalBitfield, name_hash), 4u,
  DDS_OP_RTS,

  /* MinimalSequenceType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalSequenceType, collection_flag),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalSequenceType, header), (3u << 16u) + 7u /* MinimalCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalSequenceType, element), (3u << 16u) + 9u /* MinimalCollectionElement */,
  DDS_OP_RTS,

  /* MinimalCollectionHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalCollectionHeader, common), (3u << 16u) + 65276u /* CommonCollectionHeader */,
  DDS_OP_RTS,

  /* MinimalCollectionElement */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalCollectionElement, common), (3u << 16u) + 65282u /* CommonCollectionElement */,
  DDS_OP_RTS,

  /* MinimalArrayType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalArrayType, collection_flag),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalArrayType, header), (3u << 16u) + 7u /* MinimalArrayHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalArrayType, element), (3u << 16u) + 65526u /* MinimalCollectionElement */,
  DDS_OP_RTS,

  /* MinimalArrayHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalArrayHeader, common), (3u << 16u) + 65300u /* CommonArrayHeader */,
  DDS_OP_RTS,

  /* MinimalMapType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalMapType, collection_flag),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalMapType, header), (3u << 16u) + 65510u /* MinimalCollectionHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalMapType, key), (3u << 16u) + 65512u /* MinimalCollectionElement */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalMapType, element), (3u << 16u) + 65509u /* MinimalCollectionElement */,
  DDS_OP_RTS,

  /* MinimalEnumeratedType */
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalEnumeratedType, enum_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalEnumeratedType, header), (3u << 16u) + 8u /* MinimalEnumeratedHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_MinimalEnumeratedType, literal_seq), sizeof (DDS_XTypes_MinimalEnumeratedLiteral), (4u << 16u) + 10u /* MinimalEnumeratedLiteral */,
  DDS_OP_RTS,

  /* MinimalEnumeratedHeader */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalEnumeratedHeader, common), (3u << 16u) + 65306u /* CommonEnumeratedHeader */,
  DDS_OP_RTS,

  /* MinimalEnumeratedLiteral */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalEnumeratedLiteral, common), (3u << 16u) + 65312u /* CommonEnumeratedLiteral */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalEnumeratedLiteral, detail), (3u << 16u) + 65429u /* MinimalMemberDetail */,
  DDS_OP_RTS,

  /* MinimalBitmaskType */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_2BY, offsetof (DDS_XTypes_MinimalBitmaskType, bitmask_flags),
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalBitmaskType, header), (3u << 16u) + 65520u /* MinimalEnumeratedHeader */,
  DDS_OP_ADR | DDS_OP_TYPE_SEQ | DDS_OP_SUBTYPE_STU, offsetof (DDS_XTypes_MinimalBitmaskType, flag_seq), sizeof (DDS_XTypes_MinimalBitflag), (4u << 16u) + 5u /* MinimalBitflag */,
  DDS_OP_RTS,

  /* MinimalBitflag */
  DDS_OP_DLC,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalBitflag, common), (3u << 16u) + 65318u /* CommonBitflag */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_MinimalBitflag, detail), (3u << 16u) + 65410u /* MinimalMemberDetail */,
  DDS_OP_RTS,

  /* TypeIdentifierPair */
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_TypeIdentifierPair, type_identifier1), (3u << 16u) + 64673u /* TypeIdentifier */,
  DDS_OP_ADR | DDS_OP_TYPE_EXT, offsetof (DDS_XTypes_TypeIdentifierPair, type_identifier2), (3u << 16u) + 64670u /* TypeIdentifier */,
  DDS_OP_RTS
};

const dds_topic_descriptor_t DDS_XTypes_TypeMapping_desc =
{
  .m_size = sizeof (DDS_XTypes_TypeMapping),
  .m_align = 8u,
  .m_flagset = DDS_TOPIC_NO_OPTIMIZE | DDS_TOPIC_CONTAINS_UNION,
  .m_nkeys = 0u,
  .m_typename = "DDS::XTypes::TypeMapping",
  .m_keys = NULL,
  .m_nops = 369,
  .m_ops = DDS_XTypes_TypeMapping_ops,
  .m_meta = ""
};
