"""Configuration file for the Sphinx documentation builder.

For the full list of built-in configuration values, see the documentation:
https://www.sphinx-doc.org/en/master/usage/configuration.html
"""

from __future__ import annotations

from os import environ
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parent / "_ext"))

project = "Cahute"
version = "0.6"
copyright = "2024-2025, Thomas Touhey"
author = "Thomas Touhey"

major, minor, *_ = version.split(".")
major, minor = int(major), int(minor)
if major == 0:
    public_version = f"{major}.{minor}"
else:
    public_version = f"{major}"

extensions = [
    "sphinx.ext.intersphinx",
    "sphinx.ext.todo",
    "sphinxcontrib.mermaid",
    "cahute_extensions",
]

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]
primary_domain = "c"
rst_epilog = rf"""
.. |shared_pkg| replace:: ``cahute-{public_version}``
.. |static_pkg| replace:: ``cahute-{public_version}-static``
.. |public_version| replace:: {public_version}
"""

html_theme = "furo"
html_theme_options = {
    "light_css_variables": {
        "color-announcement-text": "#000000",
        "color-announcement-background": "#F9B308",
    },
    "dark_css_variables": {
        "color-announcement-text": "#000000",
        "color-announcement-background": "#F9B308",
    },
    "footer_icons": [
        {
            "name": "Planète Casio",
            "url": "https://www.planet-casio.com/Fr/",
            "html": (Path("_static") / "planete_casio.svg").open().read(),
            "class": ""
        },
        {
            "name": "Made by a Human",
            "url": "/project.html",
            "html": (Path("_static") / "made_by_a_human.svg").open().read(),
            "class": "",
        },
    ],
}
html_extra_path = [str(Path("_static") / "cover.svg")]

if environ.get("IS_PREVIEW"):
    html_theme_options["announcement"] = (
        "<p>This is a <b>preview</b> of the documentation for the next "
        + "version of Cahute.</p><p>It may describe features that are not "
        + "available in the latest release.</p>"
    )

html_static_path = ["_static"]
html_title = f"Cahute {version}"
html_favicon = '_static/favicon.png'
html_logo = '_static/cahute.svg'
html_use_index = False
html_copy_source = False
html_show_sourcelink = False
html_domain_indices = False
html_css_files = ["custom6.css"]

intersphinx_mapping = {}

todo_include_todos = True

mermaid_output_format = "raw"
mermaid_use_local = "https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.esm.min.mjs"
mermaid_init_js = """
import mermaid from "https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.esm.min.mjs";

function isDarkMode() {
    const color = (
        getComputedStyle(document.body)
        .getPropertyValue("--color-code-foreground")
    );

    if (color == "#d0d0d0")
        return true;

    return false;
}

function initializeMermaid(isStart) {
    mermaid.initialize({
        startOnLoad: isStart,
        theme: isDarkMode() ? "dark" : "base",
        darkMode: isDarkMode(),
        securityLevel: "antiscript"
    });
}

const observer = new MutationObserver(function(mutations) {
    mutations.forEach(function(mutation) {
        if (
            mutation.type != "attributes"
            || mutation.attributeName != "data-theme"
        )
            return;

        const nodes = document.querySelectorAll(".mermaid");
        nodes.forEach(node => {
            /* Restore the original code before reprocessing. */
            node.innerHTML = node.getAttribute("data-original-code");

            /* Remove the attribute saying data is processed; it is not! */
            if (node.hasAttribute("data-processed"))
                node.removeAttribute("data-processed");
        });

        initializeMermaid(false);
        mermaid.run({nodes: nodes, querySelector: ".mermaid"});
    });
});

(function (window) {
    /* Store original code for diagrams into an attribute directly, since
       Mermaid actually completely replaces the content and removes the
       original code. */
    document.querySelectorAll(".mermaid").forEach(node => {
        node.setAttribute("data-original-code", node.innerHTML);
    })

    initializeMermaid(true);
    observer.observe(document.body, {attributes: true});
})(window);
"""

redirects = {
    "/install-guides/": "/guides/install/",
    "/build-guides/": "/guides/build/",
    "/contribution-guides.html": "/guides/contribution.html",
    "/guides/contribute.html": "/guides/contribution/contribute.html",
    "/guides/report.html": "/guides/contribution/report.html",
    "/guides/report-feature.html": "/guides/contribution/report-feature.html",
    "/guides/package.html": "/guides/contribution/package.html",
    "/guides/create-merge-request.html": "/guides/contribution/create-merge-request.html",
    "/cli-guides.html": "/guides/cli.html",
    "/cli-guides/": "/guides/cli/",
    "/developer-guides.html": "/guides/developer.html",
    "/developer-guides/": "/guides/developer/",
    "/misc-guides.html": "/guides/misc.html",
    "/misc-guides/": "/guides/misc/",
    "/data-formats.html": "/topics/data-formats.html",
    "/topics/number-formats.html": "/topics/data-formats/number-formats.html",
    "/topics/picture-formats.html": "/topics/data-formats/picture-formats.html",
    "/topics/text-encodings.html": "/topics/data-formats/text-encodings.html",
    "/communication-protocols.html": "/topics/communication.html",
    "/topics/communication-protocols.html": "/topics/communication.html",
    "/topics/protocols/": "/topics/communication/",
    "/topics/usb-detection.html": "/topics/communication/usb-detection.html",
    "/topics/communication-protocols/": "/topics/communication/",
    "/topics/communication-protocols/rationales.html": "/topics/communication/purposes.html",
    "/topics/communication/rationales.html": "/topics/communication/purposes.html",
    "/features.html": "/topics/features.html",
    "/topics/systems.html": "/topics/features/systems.html",
    "/topics/contexts.html": "/topics/features/contexts.html",
    "/topics/links.html": "/topics/features/links.html",
    "/topics/files.html": "/topics/features/files.html",
    "/topics/data.html": "/topics/features/data.html",
    "/topics/logging.html": "/topics/features/logging.html",
    "/internals.html": "/topics/internals.html",
    "/internals/": "/topics/internals/",
    "/cli.html": "/references/cli.html",
    "/cli/": "/references/cli/",
    "/headers.html": "/references/headers.html",
    "/headers/": "/references/headers/",
    "/cmake.html": "/references/cmake.html",
    "/project.html": "/topics/project.html",
    "/project/": "/topics/project/",
}
