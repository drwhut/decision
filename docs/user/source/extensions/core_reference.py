from docutils import nodes
from docutils.parsers.rst import Directive

import json

class CoreReference(Directive):

    def create_warning(self, message):
        warning = nodes.warning()
        warning += nodes.paragraph(text=message)
        return warning
    
    """
    TODO:
    For the sockets, I tried to make them tables. Really, I genuinely tried.
    But MY GOD the Docutils documentation is baaaddd. Not only does it not have
    any styling, but THERE IS LITERALLY NOTHING DOCUMENTED ABOUT TABLES.

    So, if you know how the hell tables work in Docutils, please let me know!
    Your knowledge and wisdom would be greatly appreciated!
    """
    
    def parse_socket(self, socket, isInput):
        header = "%s (%s)" % (socket["name"], socket["type"])

        i = nodes.list_item()
        i += nodes.paragraph(text=header)

        body = nodes.bullet_list()
        
        desc = nodes.list_item()
        desc += nodes.paragraph(text=socket["description"])

        if isInput:
            default = nodes.list_item()
            defaultText = "Default value: " + str(socket["default"])
            default += nodes.paragraph(text=defaultText)

        if isInput:
            body += [desc, default]
        else:
            body += [desc]

        i += [body]
        return i
    
    def parse_sockets(self, sockets, isInput):
        l = nodes.enumerated_list()
        l += [self.parse_socket(socket, isInput) for socket in sockets]
        return l

    def parse_definition(self, definition):
        name = definition["name"]
        description = definition["description"]

        d = nodes.section(ids=[name])
        d += nodes.title(name, name)

        d += nodes.paragraph(text=description)

        if definition["infiniteInputs"] == "true":
            d += nodes.paragraph(text="This node allows for an infinite number of inputs.")
        
        inputs = nodes.section(ids=[name + "-inputs"])
        inputs += nodes.title("Inputs", "Inputs")

        inputs += self.parse_sockets(definition["inputs"], True)

        d += [inputs]
        
        outputs = nodes.section(ids=[name + "-outputs"])
        outputs += nodes.title("Outputs", "Outputs")

        outputs += self.parse_sockets(definition["outputs"], False)

        d += [outputs]

        return d
    
    def parse_json(self, core):
        functions = nodes.section(ids=["functions"])
        functions += nodes.title("Core Functions", "Core Functions")

        functions += [self.parse_definition(d) for d in core["functions"]]

        subroutines = nodes.section(ids=["subroutines"])
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