from docutils import nodes
from docutils.parsers.rst import Directive

import json

class CoreReference(Directive):

    def create_warning(self, message):
        warning = nodes.warning()
        warning += nodes.paragraph(text=message)
        return warning
    
    def parse_sockets(self, sockets, input):
        return nodes.paragraph(text="Placeholder.")

    def parse_definition(self, definition):
        name = definition["name"]
        description = definition["description"]

        d = nodes.section(ids=nodes.make_id("definition-" + name))
        d += nodes.title(name, name)

        d += nodes.paragraph(text=description)

        if definition["infiniteInputs"] == "true":
            d += nodes.paragraph(text="This node allows for an infinite number of inputs.")
        
        inputs = nodes.section(ids=nodes.make_id("inputs-" + name))
        inputs += nodes.title("Inputs", "Inputs")

        inputs += self.parse_sockets(definition["inputs"], True)

        d += [inputs]
        
        outputs = nodes.section(ids=nodes.make_id("outputs-" + name))
        outputs += nodes.title("Outputs", "Outputs")

        outputs += self.parse_sockets(definition["outputs"], False)

        d += [outputs]

        return d
    
    def parse_json(self, core):
        functions = nodes.section(ids=nodes.make_id("core-functions"))
        functions += nodes.title("Core Functions", "Core Functions")

        functions += [self.parse_definition(d) for d in core["functions"]]

        subroutines = nodes.section(ids=nodes.make_id("core-subroutines"))
        subroutines += nodes.title("Core Subroutines", "Core Subroutines")

        subroutines += [self.parse_definition(d) for d in core["subroutines"]]

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