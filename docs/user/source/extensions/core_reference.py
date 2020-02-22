from docutils import nodes
from docutils.parsers.rst import Directive

class CoreReference(Directive):

    def run(self):
        return [nodes.paragraph(text="Hi!")]

def setup(app):
    app.add_directive("core-reference", CoreReference)

    return {
        "version": "0.1",
        "parallel_read_safe": False,
        "parallel_write_safe": False
    }