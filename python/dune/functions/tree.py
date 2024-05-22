from __future__ import absolute_import, division, print_function, unicode_literals


class Tree(object):
    def __init__(self, name, children=None):
        self.name = name
        self.children = []
        if children is not None:
            assert(all(isinstance(c, Tree) for c in children))
            self.children = list(children)

    def __mul__(self, other):
        return Composite(self, other)

    def __pow__(self, p):
        return Power(self, p)

    def __eq__(self, other):
        if isinstance(other, Tree):
            return (self.name == other.name) and (self.children == other.children)
        return False

    def __getitem__(self, *item):
        if len(item) == 1 and isinstance(item[0], (list, tuple)):
            item = item[0]

        if len(item) == 0:
            return self

        if isinstance(item[0], int):
            if 0 <= item[0] < len(self.children):
                return self.children[item[0]][item[1:]]
            else:
                raise KeyError("Index must be between 0 and %s but was %s" % (len(self.children)-1, item))
        else:
            raise ValueError("Invalid index %s! Only integers or list/tuples of integers are allowed" % item)


    def __hash__(self):
        return hash((self.name, tuple(self.children)))

class Lagrange(Tree):
    def __init__(self, order, dimRange=1):
        Tree.__init__(self, "Lagrange")
        self.order = order
        self.dimRange = dimRange

    def __repr__(self):
        if self.dimRange == 1:
            return "Lagrange<" + str(self.order) + ">"
        else:
            return "Lagrange<" + str(self.order) + ">^" + str(self.dimRange)

    def __eq__(self, other):
        if isinstance(other, Lagrange):
            return (super().__eq__(other)) and (self.order == other.order) and (self.dimRange == other.dimRange)
        return False

    def __hash__(self):
        return hash((super().__hash__(), self.order, self.dimRange))


class DG(Tree):
    def __init__(self, order, dimRange=1):
        Tree.__init__(self, "DG")
        self.order = order
        self.dimRange = dimRange

    def __repr__(self):
        if self.dimRange == 1:
            return "DG<" + str(self.order) + ">"
        else:
            return "DG<" + str(self.order) + ">^" + str(self.dimRange)

    def __eq__(self, other):
        if isinstance(other, DG):
            return (super().__eq__(other)) and (self.order == other.order) and (self.dimRange == other.dimRange)
        return False

    def __hash__(self):
        return hash((super().__hash__(), self.order, self.dimRange))


class Composite(Tree):
    def __init__(self, *args, **kwargs):
        assert len(args) > 0
        Tree.__init__(self, "Composite", args)
        self.blocked = kwargs.get("blocked", False)
        self.layout = kwargs.get("layout", "lexicographic")

    def __repr__(self):
        return "(" + " * ".join(repr(c) for c in self.children) + ")"

    def __eq__(self, other):
        if isinstance(other, Composite):
            return (super().__eq__(other)) and (self.blocked == other.blocked) and (self.layout == other.layout)
        return False

    def __hash__(self):
        return hash((super().__hash__(), self.blocked, self.layout))


class Power(Tree):
    def __init__(self, children, exponent, **kwargs):
        assert children is not None
        Tree.__init__(self, "Power", [children])
        assert len(self.children) == 1
        self.exponent = exponent
        self.blocked = kwargs.get("blocked", False)
        self.layout = kwargs.get("layout", "lexicographic")

    def __repr__(self):
        if self.exponent == 1:
            return repr(self.children[0])
        else:
            return "[" + repr(self.children[0]) + "]^" + str(self.exponent)

    def __eq__(self, other):
        if isinstance(other, Power):
            return (super().__eq__(other)) and (self.exponent == other.exponent)
        return False

    def __getitem__(self, *item):
        if len(item) == 1 and isinstance(item[0], (list, tuple)):
            item = item[0]

        if len(item) == 0:
            return self

        if isinstance(item[0], int):
            if item[0] in range(self.exponent):
                return self.children[0][item[1:]]
            else:
                raise KeyError("Index must be between 0 and %s but was %s" % (self.exponent-1, item))
        else:
            raise ValueError("Invalid index %s! Only integers or list/tuples of integers are allowed" % item)

        return self[item[0]][item[1:]]

    def __hash__(self):
        return hash((super().__hash__(), self.exponent))

def toFem(tree, dimGrid):
    assert isinstance(tree, Tree)
    if isinstance(tree, Lagrange):
        return "LagrangeDiscreteFunctionSpace< FunctionSpace< " + str(dimGrid) + ", " + str(tree.dimRange) + " >, " + str(tree.order) + " >"
    elif isinstance(tree, DG):
        return "DiscontinuousGalerkinSpace< FunctionSpace< " + str(dimGrid) + ", " + str(tree.dimRange) + " >, " + str(tree.order) + " >"
    elif isinstance(tree, Composite):
        return "TupleDiscreteFunctionSpace< " + ", ".join(toFem(c, dimGrid) for c in tree.children) + " >"
    elif isinstance(tree, Power):
        return "PowerDiscreteFunctionSpace< " + toFem(tree.children[0], dimGrid) + ", " + str(tree.exponent) + " >"
    else:
        raise Exception("Unknown type of tree: " + repr(tree))
