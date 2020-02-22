from docutils import nodes
from docutils.parsers.rst import Directive

import json

class CoreReference(Directive):

    def create_warning(self, message):
        warning = nodes.warning()
        warning += nodes.paragraph(text=message)
        return warning
    
    def parse_json(self, core):
        functions = nodes.section(ids=nodes.make_id("core-functions"))
        functions += nodes.title("Core Functions", "Core Functions")

        subroutines = nodes.section(ids=nodes.make_id("core-subroutines"))
        subroutines += nodes.title("Core Subroutines", "Core Subroutines")

        return [functions, subroutines]

    def run(self):

        coreJSON = ""

        # Firstly, try and open the core.json file.
        try:
            f = open("core.json", "r")
            coreJSON = f.read()
            f.close()
        except Exception as e:
            return [self.create_warning("Cannot open the core.json file! %s" % str(e))]
        
        # Next, try and decode the JSON.
        try:
            core = json.loads(coreJSON)
        except Exception as e:
            return [self.create_warning("JSON is invalid! %s" % str(e))]
        
        # Finally, parse the JSON.
        return self.parse_json(core)

def setup(app):
    app.add_directive("core-reference", CoreReference)

    return {
        "version": "0.3.0",
        "parallel_read_safe": False,
        "parallel_write_safe": False
    }