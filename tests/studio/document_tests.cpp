#include "gspl/studio/document.hpp"
#include <cassert>
#include <cstdio>

void test_document_kind() {
    gspl::studio::Document doc(gspl::studio::DocumentKind::Text, "main.gspl");
    assert(doc.kind() == gspl::studio::DocumentKind::Text);
    assert(doc.name() == "main.gspl");
    assert(!doc.is_dirty());
}

void test_document_dirty() {
    gspl::studio::Document doc(gspl::studio::DocumentKind::Gene, "genes");
    assert(!doc.is_dirty());
    doc.set_dirty(true);
    assert(doc.is_dirty());
    doc.set_dirty(false);
    assert(!doc.is_dirty());
}

void test_document_change_callback() {
    gspl::studio::Document doc(gspl::studio::DocumentKind::Form, "form");
    bool called = false;
    doc.set_change_callback([&](gspl::studio::DocumentChange change) {
        called = true;
        assert(change == gspl::studio::DocumentChange::Dirty);
    });
    doc.set_dirty(true);
    assert(called);
}

void test_document_file_path() {
    gspl::studio::Document doc(gspl::studio::DocumentKind::Text, "test.gspl");
    doc.set_file_path("/path/to/test.gspl");
    assert(doc.file_path() == "/path/to/test.gspl");
}

int main() {
    test_document_kind();
    test_document_dirty();
    test_document_change_callback();
    test_document_file_path();
    std::printf("All document tests passed.\n");
    return 0;
}
