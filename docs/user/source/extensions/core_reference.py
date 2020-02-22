from docutils import nodes
from docutils.parsers.rst import Directive

class CoreReference(Directive):

    def run(self):
        functions = nodes.section(ids=nodes.make_id("core-functions"))
        functions += nodes.title("Core Functions", "Core Functions")

        subroutines = nodes.section(ids=nodes.make_id("core-subroutines"))
        subroutines += nodes.title("Core Subroutines", "Core Subroutines")

        return [functions, subroutines]

def setup(app):
    app.add_directive("core-reference", CoreReference)

    return {
        "version": "0.3.0",
        "parallel_read_safe": False,
        "parallel_write_safe": False
    }