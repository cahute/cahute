# *****************************************************************************
# Copyright (C) 2024-2026 Thomas Touhey <thomas@touhey.fr>
#
# This software is governed by the CeCILL 2.1 license under French law and
# abiding by the rules of distribution of free software. You can use, modify
# and/or redistribute the software under the terms of the CeCILL 2.1 license
# as circulated by CEA, CNRS and INRIA at the following
# URL: https://cecill.info
#
# As a counterpart to the access to the source code and rights to copy, modify
# and redistribute granted by the license, users are provided only with a
# limited warranty and the software's author, the holder of the economic
# rights, and the successive licensors have only limited liability.
#
# In this respect, the user's attention is drawn to the risks associated with
# loading, using, modifying and/or developing or reproducing the software by
# the user in light of its specific status of free software, that may mean
# that it is complicated to manipulate, and that also therefore means that it
# is reserved for developers and experienced professionals having in-depth
# computer knowledge. Users are therefore encouraged to load and test the
# software's suitability as regards their requirements in conditions enabling
# the security of their systems and/or data to be ensured and, more generally,
# to use and operate it in the same conditions as regards security.
#
# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL 2.1 license and that you accept its terms.
# *****************************************************************************
"""Sphinx plugin to add the "seven-command" reStructuredText directive."""

from __future__ import annotations

import re
from pathlib import Path
from typing import Callable, TYPE_CHECKING

import docutils.nodes as nodes
from docutils.parsers.rst import Directive
from docutils.parsers.rst.directives import unchanged
from docutils.statemachine import StringList
from docutils.utils import SystemMessagePropagation
from sphinx import addnodes
from sphinx.directives import SphinxDirective
from sphinx.writers.html import HTMLTranslator

if TYPE_CHECKING:
    from sphinx.application import Sphinx


class feature_list(nodes.General, nodes.Element):
    pass


class feature(nodes.General, nodes.Element):
    pass


class feature_icons(nodes.General, nodes.Element):
    pass


class feature_detail(nodes.General, nodes.Element):
    pass


class feature_title(nodes.General, nodes.Element):
    pass


class system_list(nodes.General, nodes.Element):
    pass


class system(nodes.General, nodes.Element):
    pass


class system_name(nodes.General, nodes.Element):
    pass


class system_install_guides(nodes.General, nodes.Element):
    pass


class system_build_guides(nodes.General, nodes.Element):
    pass


class system_guides_title(nodes.General, nodes.TextElement):
    def __init__(self, rawsource: str = "", *children) -> None:
        nodes.General.__init__(self)
        nodes.TextElement.__init__(self, rawsource, "", *children)


class MyHTMLTranslator(HTMLTranslator):
    """Custom"""

    def visit_title(self, node):
        # Remove captions in toctrees, only keep them for side navigation.
        if (
            isinstance(node.parent, addnodes.compact_paragraph)
            and node.parent.get("toctree")
            and isinstance(node.parent.parent, nodes.compound)
            and "toctree-wrapper" in node.parent.parent.get("classes", ())
        ):
            raise nodes.SkipNode()

        is_title = (
            not isinstance(
                node.parent,
                (
                    nodes.topic,
                    nodes.sidebar,
                    nodes.Admonition,
                    nodes.table,
                    nodes.document,
                ),
            )
            and self.section_level == 1
        )
        self.context.append(is_title)

        if is_title:
            self.body.append(self.starttag(node, "div", "", CLASS="title-container"))
            self.body.append(self.starttag(node, "div", "", CLASS="title-contrast"))
            self.body.append(self.starttag(node, "hr", ""))
            self.body.append("</hr></div>")

        super().visit_title(node)

    def depart_title(self, node):
        super().depart_title(node)

        if self.context.pop():
            self.body.append("</div>")

    def visit_definition_list_item(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="dl-item"))

    def depart_definition_list_item(self, node):
        self.body.append("</div>")

    def visit_feature_list(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="feature-list"))

    def depart_feature_list(self, node):
        self.body.append("</div>")

    def visit_feature(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="feature"))

    def depart_feature(self, node):
        self.body.append("</div>")

    def visit_feature_icons(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="feature-icons"))

    def depart_feature_icons(self, node):
        self.body.append("</div>")

    def visit_feature_detail(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="feature-detail"))

    def depart_feature_detail(self, node):
        self.body.append("</div>")

    def visit_feature_title(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="feature-title"))

    def depart_feature_title(self, node):
        self.body.append("</div>")

    def visit_system_list(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="system-list"))

    def depart_system_list(self, node):
        self.body.append("</div>")

    def visit_system(self, node):
        pass

    def depart_system(self, node):
        pass

    def visit_system_name(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="system-name"))

    def depart_system_name(self, node):
        self.body.append("</div>")

    def visit_system_install_guides(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="system-install-guides"))

    def depart_system_install_guides(self, node):
        self.body.append("</div>")

    def visit_system_build_guides(self, node):
        self.body.append(self.starttag(node, "div", "", CLASS="system-build-guides"))

    def depart_system_build_guides(self, node):
        self.body.append("</div>")

    def visit_system_guides_title(self, node):
        self.body.append(self.starttag(node, "p", "", CLASS="system-guides-title"))

    def depart_system_guides_title(self, node):
        self.body.append("</p>")


def ascii_hex(
    *,
    size: int,
    default: int | None = None,
) -> Callable[[str | None], int]:
    """Convert the argument into an ASCII-HEX number.

    :param size: Size of the expected ASCII-HEX.
    """
    if default is not None and (default < 0 or default >= 2**size):
        raise ValueError(f"default must be between 0 and {2**size - 1}")

    def func(argument: str | None) -> int:
        """Convert the argument into a short hexadecimal number."""
        if argument is None:
            if default is None:
                raise ValueError("missing value")

            return default

        m = re.fullmatch(
            r"[0-9A-Za-z]{" + re.escape(str(size)) + "}",
            argument,
        )
        if m is None:
            raise ValueError(f"expected a {size}-digit hex")

        return int(m.group().upper(), 16)

    return func


class SevenCommandDirective(Directive):
    """Directive for documenting a Protocol 7.00 command."""

    has_content = True
    option_spec = {
        "code": ascii_hex(size=2),
        "ow": unchanged,
        "ow-example": ascii_hex(size=2, default=0),
        "dt": unchanged,
        "dt-example": ascii_hex(size=2, default=0),
        "fs": unchanged,
        "fs-example": ascii_hex(size=8, default=0),
        "d1": unchanged,
        "d1-example": unchanged,
        "d2": unchanged,
        "d2-example": unchanged,
        "d3": unchanged,
        "d3-example": unchanged,
        "d4": unchanged,
        "d4-example": unchanged,
        "d5": unchanged,
        "d5-example": unchanged,
        "d6": unchanged,
        "d6-example": unchanged,
    }

    def is_payload_required(self) -> bool:
        """Whether at least one field is used for the command."""
        return bool(
            self.options.get("ow")
            or self.options.get("dt")
            or self.options.get("fs")
            or self.options.get("d1")
            or self.options.get("d2")
            or self.options.get("d3")
            or self.options.get("d4")
            or self.options.get("d5")
            or self.options.get("d6")
        )

    def get_table(self) -> nodes.table:
        """Get the table."""
        table = nodes.table()
        table["classes"] += ["colwidths-auto"]
        tgroup = nodes.tgroup(cols=2)
        table += tgroup

        tbody = nodes.tbody()
        tgroup += nodes.colspec()
        tgroup += nodes.colspec()

        def add_entry(name: str, description: str, /) -> None:
            """Add an entry to the table."""
            nonlocal tbody

            row_node = nodes.row()
            tbody += row_node

            title_entry_node = nodes.entry()
            desc_entry_node = nodes.entry()
            row_node += title_entry_node
            row_node += desc_entry_node

            title = nodes.strong()
            title += [nodes.Text(name)]
            title_entry_node += [title]

            self.state.nested_parse(
                StringList([description]),
                self.content_offset,
                desc_entry_node,
            )

        if self.options.get("ow"):
            add_entry("Overwrite (OW)", self.options["ow"])
        if self.options.get("dt"):
            add_entry("Data Type (DT)", self.options["dt"])
        if self.options.get("fs"):
            add_entry("Filesize (FS)", self.options["fs"])
        for k in range(1, 7):
            if self.options.get(f"d{k}"):
                add_entry(f"Data {k} (D{k})", self.options[f"d{k}"])

        tgroup += tbody
        return table

    def get_example(self) -> str:
        """Get an example to display."""
        code = self.options["code"]
        ow = self.options.get("ow-example") or 0
        dt = self.options.get("dt-example") or 0
        fs = self.options.get("fs-example") or 0
        d1 = self.options.get("d1-example") or ""
        d2 = self.options.get("d2-example") or ""
        d3 = self.options.get("d3-example") or ""
        d4 = self.options.get("d4-example") or ""
        d5 = self.options.get("d5-example") or ""
        d6 = self.options.get("d6-example") or ""

        header = "T ST "
        content = f". {code:02X} "

        if self.is_payload_required():
            # Compute the unordered string to compute the checksum and the
            # size of the data.
            raw_unordered = f"{ow:02X}{dt:02X}{fs:08X}" + "".join(
                f"{len(x):02X}{x}" for x in (d1, d2, d3, d4, d5, d6)
            )
            data_size = len(raw_unordered)
            raw_unordered += f"{code:02X}1{data_size:04X}"

            checksum = (~sum(raw_unordered.encode("ascii")) + 1) & 255

            header += "EX DS   OW DT FS       "
            header += " ".join(f"SD{i}" for i in range(1, 7))
            content += f" 1 {data_size:04X} {ow:02X} {dt:02X} {fs:08X} "
            content += " ".join(f" {len(x):02X}" for x in (d1, d2, d3, d4, d5, d6))

            for i, x in enumerate((d1, d2, d3, d4, d5, d6)):
                if not x:
                    continue

                # We need to align the string and the header, even if the
                # string is only 1 character long.
                if len(x) == 1:
                    x = " " + x

                header += f" D{i + 1}" + " " * (len(x) - 2)
                content += f" {x}"
        else:
            header += "EX "
            content += " 0 "

            checksum = (~sum(f"{code:02X}0".encode("ascii")) + 1) & 255

        header += " CS"
        content += f" {checksum:02X}"

        return header + "\n" + content

    def run(self) -> list[nodes.Node]:
        """Run the directive.

        :return: Produced nodes.
        """
        container = nodes.container()

        if self.is_payload_required():
            container += self.get_table()

        if self.content:
            self.state.nested_parse(self.content, self.content_offset, container)

        paragraph_node = nodes.paragraph()
        container += paragraph_node
        paragraph_node += [
            nodes.Text("An example of such command is the following:"),
        ]

        example_node = nodes.literal_block()
        example_node["language"] = "text"
        container += example_node
        example_node += [nodes.Text(self.get_example())]

        return [container]


class TwoLevelListDirective(SphinxDirective):
    """Abstract directive for getting two-level bullet lists."""

    has_content = True

    def run(self, /) -> list[nodes.Node]:
        try:
            elements = self.get_elements()
            return [self.get_result(elements)]
        except SystemMessagePropagation as exc:
            return [exc.args[0]]

    def get_elements(self, /) -> list[tuple[list[nodes.Node], ...]]:
        """Get the elements from the content list.

        :return: List of elements.
        """
        if not self.content:
            error = self.reporter.error(
                f'The "{self.name}" directive is empty; content required.',
                nodes.literal_block(self.block_text, self.block_text),
                line=self.lineno,
            )
            raise SystemMessagePropagation(error)

        node = nodes.Element()
        self.state.nested_parse(self.content, self.content_offset, node)

        if len(node) != 1 or not isinstance(node[0], nodes.bullet_list):
            error = self.reporter.error(
                f'Error parsing content block for the "{self.name}" '
                + "directive: exactly one bullet list expected.",
                nodes.literal_block(self.block_text, self.block_text),
                line=self.lineno,
            )
            raise SystemMessagePropagation(error)

        list_node = node[0]
        elements = []
        for item_index, item_node in enumerate(list_node):
            if len(item_node) != 1 or not isinstance(item_node[0], nodes.bullet_list):
                error = self.reporter.error(
                    f'Error parsing content block for the "{self.name}" '
                    + "directive: two-level bullet list expected, but row "
                    + f"{item_index} does not contain a second-level bullet "
                    + "list.",
                    nodes.literal_block(self.block_text, self.block_text),
                    line=self.lineno,
                )
                raise SystemMessagePropagation(error)

            elements.append(tuple(list(subelement) for subelement in item_node[0]))

        return elements

    def get_result(self, elements: tuple[list[nodes.Node], ...], /) -> nodes.Element:
        """Get the result.

        :param elements: Elements to compute the result from.
        :return: Obtained result.
        """
        raise NotImplementedError()


class FeatureListDirective(TwoLevelListDirective):
    """Directive for listing features with icons."""

    def get_result(
        self,
        elements: tuple[list[nodes.Node], ...],
        /,
    ) -> nodes.Element:
        """Get the result.

        :param elements: Elements to compute the result from.
        :return: Obtained result.
        """
        container = feature_list()
        for *icon_elements, title, detail in elements:
            feat = feature("")
            feat += [
                feature_icons(
                    "",
                    *(
                        icon
                        for icon_element in icon_elements
                        for icon_paragraph in icon_element
                        for icon in icon_paragraph
                    ),
                ),
                feature_detail(
                    "",
                    feature_title("", *title),
                    *detail,
                ),
            ]
            container.append(feat)

        return container


class SystemGuidesDirective(TwoLevelListDirective):
    """Directive for listing supported systems and their related guides."""

    def get_result(
        self,
        elements: tuple[list[nodes.Node], ...],
        /,
    ) -> nodes.Element:
        """Get the result.

        The resulting structure here must be the following::

            <system_list>
                <system>
                    <system_name>
                        ... icon...
                        ... name...
                    </system_name>
                    <system_install_guides>
                        ... install guide intro "Install Cahute..."...
                        ... install guide list...
                    </system_install_guides>
                    <system_build_guides>
                        ... build guide intro "Build Cahute..."...
                        ... build guide list...
                    </system_build_guides>
                </system>
            </system_list>

        :param elements: Elements to compute the result from.
        :return: Obtained result.
        """
        container = system_list()
        for icon_element, name_element, *guides in elements:
            if (
                len(guides) < 1
                or len(guides[0]) < 1
                or not isinstance(
                    (install_guides_element := guides[0][0]),
                    nodes.bullet_list,
                )
            ):
                install_guides_element = nodes.paragraph(
                    "",
                    "",
                    nodes.emphasis(
                        "",
                        nodes.Text(
                            "No installation guide available for this system.",
                        ),
                    ),
                )

            if (
                len(guides) < 2
                or len(guides[1]) < 1
                or not isinstance(
                    (build_guides_element := guides[1][0]),
                    nodes.bullet_list,
                )
            ):
                build_guides_element = nodes.paragraph(
                    "",
                    "",
                    nodes.emphasis(
                        "",
                        nodes.Text(
                            "No build guide available for this system.",
                        ),
                    ),
                )

            sys = system(
                "",
                system_name(
                    "",
                    nodes.paragraph(
                        "",
                        "",
                        *(
                            icon
                            for icon_paragraph in icon_element
                            for icon in icon_paragraph
                        ),
                        *(
                            name
                            for name_paragraph in name_element
                            for name in name_paragraph
                        ),
                    ),
                ),
                system_install_guides(
                    "",
                    system_guides_title(
                        "",
                        nodes.Text("Install Cahute…"),
                    ),
                    install_guides_element,
                ),
                system_build_guides(
                    "",
                    system_guides_title(
                        "",
                        nodes.Text("Build Cahute from source…"),
                    ),
                    build_guides_element,
                ),
            )

            container.append(sys)

        return container


def add_redirects(app: Sphinx, exc: Exception | None) -> None:
    """Get iterables of pages to write, which we do not have."""
    if exc is not None or not app.builder.name.endswith("html"):
        return

    redirects: list[tuple[str, str]] = [
        (
            "/" + key.removeprefix("/"),
            value.removesuffix("/") + "/"[: key.endswith("/")],
        )
        for key, value in app.config.redirects.items()
    ]
    redirects.sort(key=lambda x: (len(x[0]), x[0]))

    with (Path(app.outdir) / "_redirects.nginx").open("w+") as fp:
        for orig, dest in redirects:
            if orig.endswith("/"):
                inst = f"rewrite ^{re.escape(orig)}(.*)$ {dest}$1 permanent;\n"
            else:
                inst = f"rewrite ^{re.escape(orig)}$ {dest} permanent;\n"

            fp.write(inst)


def setup(app: Sphinx, /) -> None:
    """Set up the extension.

    :param app: The Sphinx application to set up the extension for.
    """
    app.set_translator("html", MyHTMLTranslator)
    app.add_directive("seven-command", SevenCommandDirective)
    app.add_directive("feature-list", FeatureListDirective)
    app.add_directive("system-guides", SystemGuidesDirective)
    app.connect("build-finished", add_redirects)
    app.add_config_value("redirects", {}, "env")
    return {"parallel_read_safe": True}
