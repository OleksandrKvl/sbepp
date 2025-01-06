find_package(Doxygen REQUIRED)

set(doxygen_input
    "${PROJECT_SOURCE_DIR}/sbepp"
    "${PROJECT_SOURCE_DIR}/doc/overview.md"
    "${PROJECT_SOURCE_DIR}/doc/installation.md"
    "${PROJECT_SOURCE_DIR}/doc/sbeppc.md"
    "${PROJECT_SOURCE_DIR}/doc/integration.md"
    "${PROJECT_SOURCE_DIR}/doc/representation.md"
    "${PROJECT_SOURCE_DIR}/doc/traits.md"
    "${PROJECT_SOURCE_DIR}/doc/visit_api.md"
    "${PROJECT_SOURCE_DIR}/doc/benchmarks.md"
    "${PROJECT_SOURCE_DIR}/doc/examples.md"
    "${PROJECT_SOURCE_DIR}/doc/stringification.md"
)

set(DOXYGEN_STRIP_FROM_PATH "${PROJECT_SOURCE_DIR}/sbepp/src/sbepp")
set(DOXYGEN_HIDE_UNDOC_MEMBERS "YES")
set(DOXYGEN_HIDE_UNDOC_CLASSES "YES")
set(DOXYGEN_LAYOUT_FILE "DoxygenLayout.xml")
set(DOXYGEN_WARN_IF_UNDOCUMENTED "NO")
set(DOXYGEN_RECURSIVE "YES")
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "overview.md")
set(DOXYGEN_HTML_HEADER "header.html")
set(DOXYGEN_HTML_EXTRA_STYLESHEET "doxygen-awesome-css/doxygen-awesome.css")
set(DOXYGEN_HTML_EXTRA_FILES
    "doxygen-awesome-css/doxygen-awesome-darkmode-toggle.js"
    "doxygen-awesome-css/doxygen-awesome-paragraph-link.js"
    "doxygen-awesome-css/doxygen-awesome-interactive-toc.js"
    "version_selector_handler.js"
)
set(DOXYGEN_HTML_COLORSTYLE "LIGHT")
set(DOXYGEN_GENERATE_TREEVIEW "YES")
set(DOXYGEN_FULL_SIDEBAR "YES")
set(DOXYGEN_DISABLE_INDEX "NO")
set(DOXYGEN_MACRO_EXPANSION "YES")
set(DOXYGEN_EXPAND_ONLY_PREDEF "YES")
set(DOXYGEN_PREDEFINED
    "SBEPP_DOXYGEN"
    "SBEPP_CPP20_CONSTEXPR=constexpr"
    "SBEPP_CPP14_CONSTEXPR=constexpr"
    "SBEPP_HAS_INLINE_VARS=1"
    "SBEPP_HAS_CONCEPTS=1"
    "SBEPP_CPP17_INLINE_VAR=inline"
    "SBEPP_CPP17_NODISCARD=[[nodiscard]]"
    "SBEPP_DEPRECATED"
)
set(DOXYGEN_EXPAND_AS_DEFINED "SBEPP_BUILT_IN_IMPL")
set(DOXYGEN_WARN_AS_ERROR "FAIL_ON_WARNINGS_PRINT")
set(DOXYGEN_HTML_OUTPUT "${PROJECT_VERSION}")
set(DOXYGEN_OUTPUT_DIRECTORY "docs")

doxygen_add_docs(
    doc
    ${doxygen_input}
)