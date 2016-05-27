#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "tests.pb-c.h"
#include "tokenizer.h"

static TokenizerString to_tok_string(const ProtobufCBinaryData data) {
    TokenizerString str = {
        .length = data.len,
        .data = (char *) data.data
    };
    return str;
}

static int to_tok_state(const Suite__Test__State state) {
    #define state_case(NAME) \
        case SUITE__TEST__STATE__##NAME:\
            return html_state_##NAME;

    switch (state) {
        state_case(Data)
        state_case(PlainText)
        state_case(RCData)
        state_case(RawText)
        state_case(ScriptData)

        default: assert(false);
    }
}

static ProtobufCBinaryData to_test_string(const TokenizerString src) {
    ProtobufCBinaryData dest;
    dest.data = (unsigned char *) src.data;
    dest.len = src.length;
    return dest;
}

static void to_opt_test_string(const TokenizerOptionalString src, protobuf_c_boolean *has_value, ProtobufCBinaryData *value) {
    if ((*has_value = src.has_value)) {
        *value = to_test_string(src.value);
    }
}

static void fprint_escaped_str(FILE *file, const ProtobufCBinaryData str) {
    const size_t len = (size_t) str.len;
    const char *const data = (const char *) str.data;
    fprintf(file, "'");
    for (int i = 0; i < len; i++) {
        const char c = data[i];
        if (iscntrl(c)) {
            fprintf(file, "\\x%02X", c);
        } else if (c == '\'') {
            fprintf(file, "\\\'");
        } else {
            fprintf(file, "%c", c);
        }
    }
    fprintf(file, "'");
}

static void fprint_msg(FILE *file, const ProtobufCMessage *msg) {
    const ProtobufCMessageDescriptor *desc = msg->descriptor;
    fprintf(file, "%s { ", desc->short_name);
    const unsigned int n_fields = desc->n_fields;
    const unsigned int n_distinct_fields = desc->n_field_ranges;
    const ProtobufCFieldDescriptor* fields = desc->fields;
    const char *mem = (const char *) msg;
    for (int i = 0; i < n_fields; i++) {
        if (i > 0) {
            fprintf(file, ", ");
        }
        const ProtobufCFieldDescriptor* field = &fields[i];
        if (n_distinct_fields > 1) {
            fprintf(file, "%s = ", field->name);
        }
        if (field->flags & PROTOBUF_C_FIELD_FLAG_ONEOF) {
            unsigned int quantifier = *((unsigned int *) (mem + field->quantifier_offset));
            i += quantifier - 1;
            assert(i >= 0 && i < n_fields);
            field = &fields[i];
            for (; i < n_fields && fields[i].quantifier_offset == field->quantifier_offset; i++);
            i--;
        } else if (field->label == PROTOBUF_C_LABEL_OPTIONAL && !*((bool *) (mem + field->quantifier_offset))) {
            fprintf(file, "(none)");
            continue;
        }
        const void *value = mem + field->offset;
        unsigned int n_values = 1;
        if (field->label == PROTOBUF_C_LABEL_REPEATED) {
            n_values = *((unsigned int *) (mem + field->quantifier_offset));
            value = *((void **) value);
            fprintf(file, "[ ");
        }
        for (int j = 0; j < n_values; j++) {
            if (j > 0) {
                fprintf(file, ", ");
            }
            switch (field->type) {
                case PROTOBUF_C_TYPE_BOOL: {
                    const bool *rec = value;
                    fprintf(file, "%s", *rec ? "true" : "false");
                    value = rec + 1;
                    break;
                }

                case PROTOBUF_C_TYPE_BYTES: {
                    const ProtobufCBinaryData *rec = value;
                    fprint_escaped_str(file, *rec);
                    value = rec + 1;
                    break;
                }

                case PROTOBUF_C_TYPE_ENUM: {
                    const unsigned int *rec = value;
                    const ProtobufCEnumDescriptor *desc = field->descriptor;
                    fprintf(file, "%s", protobuf_c_enum_descriptor_get_value(desc, *rec)->name);
                    value = rec + 1;
                    break;
                }

                case PROTOBUF_C_TYPE_MESSAGE: {
                    ProtobufCMessage *const *rec = value;
                    fprint_msg(file, *rec);
                    value = rec + 1;
                    break;
                }

                default:
                    assert(false);
            }
        }
        if (field->label == PROTOBUF_C_LABEL_REPEATED) {
            fprintf(file, " ]");
        }
    }
    fprintf(file, " }");
}

typedef struct {
    bool error;
    const char *raw_pos;
    unsigned int expected_pos;
    const unsigned int expected_length;
    const Suite__Test *test;
    Suite__Test__State initial_state;
} State;

static void fprint_fail(FILE *file, const State *state, const char *msg) {
    fprintf(
        file,
        "not ok - %s\n"
        "  ---\n"
        "    input: ",
        msg
    );
    fprint_escaped_str(file, state->test->input);
    fprintf(
        file,
        "\n"
        "    state: %s\n"
        ,
        protobuf_c_enum_descriptor_get_value(&suite__test__state__descriptor, state->initial_state)->name
    );
}

static void fprint_fail_end(FILE *file) {
    fprintf(file, "  ...\n");
}

static bool tokens_match(const State *state, const Token *src, const Suite__Test__Token *expected) {
    Suite__Test__Token actual = SUITE__TEST__TOKEN__INIT;

    #define case_token(TYPE, NAME, CAP_TYPE) case token_##NAME:;\
        Suite__Test__##TYPE NAME = SUITE__TEST__##CAP_TYPE##__INIT;\
        actual.token_case = SUITE__TEST__TOKEN__TOKEN_##CAP_TYPE;\
        actual.NAME = &NAME;

    const unsigned int n_attributes = src->type == token_start_tag ? src->start_tag.attributes.count : 0;
    Suite__Test__Attribute attributes[n_attributes];
    Suite__Test__Attribute *attribute_pointers[n_attributes];

    switch (src->type) {
        default:
            assert(false);

        case_token(Character, character, CHARACTER)
            character.value = to_test_string(src->character.value);
            break;

        case_token(DocType, doc_type, DOC_TYPE)
            to_opt_test_string(src->doc_type.name, &doc_type.has_name, &doc_type.name);
            to_opt_test_string(src->doc_type.public_id, &doc_type.has_public_id, &doc_type.public_id);
            to_opt_test_string(src->doc_type.system_id, &doc_type.has_system_id, &doc_type.system_id);
            doc_type.force_quirks = src->doc_type.force_quirks;
            break;

        case_token(Comment, comment, COMMENT)
            comment.value = to_test_string(src->comment.value);
            break;

        case_token(EndTag, end_tag, END_TAG)
            end_tag.name = to_test_string(src->end_tag.name);
            break;

        case_token(StartTag, start_tag, START_TAG)
            start_tag.name = to_test_string(src->start_tag.name);
            start_tag.n_attributes = n_attributes;
            for (int i = 0; i < n_attributes; i++) {
                Suite__Test__Attribute *attr = attribute_pointers[i] = &attributes[i];
                suite__test__attribute__init(attr);
                const Attribute *src_attr = &src->start_tag.attributes.items[i];
                attr->name = to_test_string(src_attr->name);
                attr->value = to_test_string(src_attr->value);
            }
            start_tag.attributes = &attribute_pointers[0];
            start_tag.self_closing = src->start_tag.self_closing;
            break;
    }

    size_t actual_len = protobuf_c_message_get_packed_size(&actual.base);
    size_t expected_len = protobuf_c_message_get_packed_size(&expected->base);

    bool same = actual_len == expected_len;

    if (same) {
        uint8_t actual_buf[actual_len];
        uint8_t expected_buf[expected_len];

        protobuf_c_message_pack(&actual.base, actual_buf);
        protobuf_c_message_pack(&expected->base, expected_buf);

        same = memcmp(actual_buf, expected_buf, actual_len) == 0;
    }

    if (!same) {
        fprint_fail(stdout, state, "Token mismatch");

        fprintf(stdout, "    actual: ");
        fprint_msg(stdout, &actual.base);
        fprintf(stdout, "\n");

        fprintf(stdout, "    expected: ");
        fprint_msg(stdout, &expected->base);
        fprintf(stdout, "\n");

        fprint_fail_end(stdout);
    }

    return same;
}

static void on_token(const Token *token) {
    State *state = (State *) token->extra;
    if (state->error) {
        return;
    }
    if (token->raw.data != state->raw_pos) {
        fprint_fail(stdout, state, "Raw position mismatch");
        fprintf(stdout, "  actual: %p\n", token->raw.data);
        fprintf(stdout, "  expected: %p\n", state->raw_pos);
        fprint_fail_end(stdout);
        state->error = true;
        return;
    }
    if (state->expected_pos >= state->expected_length) {
        fprint_fail(stdout, state, "Extraneous tokens");
        fprintf(stdout, "  actual: %u\n", state->expected_pos);
        fprintf(stdout, "  expected: %u\n", state->expected_length);
        fprint_fail_end(stdout);
        state->error = true;
        return;
    }
    Suite__Test__Token *expected = state->test->output[state->expected_pos++];
    if (!tokens_match(state, token, expected)) {
        state->error = true;
        return;
    }
    state->raw_pos = token->raw.data + token->raw.length;
}

static void run_test(const Suite__Test *test) {
    printf(
        "# %.*s\n",
        (int ) test->description.len,
        (char *) test->description.data
    );
    for (int i = 0; i < test->input.len; i++) {
        char c = (char) test->input.data[i];
        if (c == '&' || c == '\0' || c == '\r' || c == 'A' || c == 'B' || c == 'Y' || c == 'Z') {
            printf("ok # skip Decoding is unsupported yet\n");
            return;
        }
    }
    State custom_state = {
        .expected_length = test->n_output,
        .test = test
    };
    TokenizerOpts options = {
        .on_token = on_token,
        .last_start_tag_name = to_tok_string(test->last_start_tag),
        .extra = &custom_state
    };
    TokenizerState state;
    TokenizerString input = to_tok_string(test->input);
    for (int i = 0; i < test->n_initial_states; i++) {
        custom_state.initial_state = test->initial_states[i];
        custom_state.error = false;
        custom_state.raw_pos = input.data;
        custom_state.expected_pos = 0;
        options.initial_state = to_tok_state(custom_state.initial_state);
        html_tokenizer_init(&state, &options);
        html_tokenizer_feed(&state, &input);
        // html_tokenizer_feed(&state, NULL);
        if (custom_state.error) return;
        if (state.cs == html_state_error) {
            fprint_fail(stdout, &custom_state, "Tokenization error");
            fprint_fail_end(stdout);
            return;
        }
        if (custom_state.expected_pos < custom_state.expected_length) {
            fprint_fail(stdout, &custom_state, "Not enough tokens");
            fprintf(stdout, "  actual: %u\n", custom_state.expected_pos);
            fprintf(stdout, "  expected: %u\n", custom_state.expected_length);
            fprint_fail_end(stdout);
            return;
        }
    }
    printf("ok\n");
}

static void run_suite(const Suite *suite) {
    const int n = suite->n_tests;
    printf(
        "TAP version 13\n"
        "1..%u\n",
        n
    );
    for (int i = 0; i < n; i++) {
        run_test(suite->tests[i]);
    }
}

int main() {
    FILE *infile = fopen("../tests.dat", "rb");

    assert(infile);

    fseek(infile, 0L, SEEK_END);
    unsigned int numbytes = ftell(infile);

    uint8_t *buffer = malloc(numbytes);

    assert(buffer);

    fseek(infile, 0L, SEEK_SET);

    assert(fread(buffer, sizeof(char), numbytes, infile) == numbytes);
    fclose(infile);

    Suite *suite = suite__unpack(NULL, numbytes, buffer);

    assert(suite);

    run_suite(suite);

    suite__free_unpacked(suite, NULL);

    free(buffer);
    return 0;
}