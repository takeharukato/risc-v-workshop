# -*- coding: utf-8 mode: python -*-
# gdb pythonスクリプト
# 参考: https://interrupt.memfault.com/blog/automate-debugging-with-gdb-python-api
#
from gdb.printing import PrettyPrinter, register_pretty_printer
import gdb

class RBTreeParser:
    """Parse the RBTree"""
    def __init__(self):
        self.verbose = False
    def _rbtree_root(self, val):
        node_ptr = val
        return node_ptr["rbh_root"]
    def _rbtree_parent(self, val, ent):
        node_ptr = val
        if self.verbose:
            print("_rbtree_parent args :%s %s" % (val, ent))
        tree_ent = node_ptr[ent]
        tree_left = tree_ent["rbe_parent"]
        return tree_left
    def _rbtree_left(self, val, ent):
        node_ptr = val
        if self.verbose:
            print("_rbtree_left args :%s %s" % (val, ent))
        tree_ent = node_ptr[ent]
        tree_left = tree_ent["rbe_left"]
        return tree_left
    def _rbtree_right(self, val, ent):
        node_ptr = val
        if self.verbose:
            print("_rbtree_right args :%s %s" % (val, ent))
        tree_ent = node_ptr[ent]
        tree_right = tree_ent["rbe_right"]
        return tree_right

    def _rbtree_minmax(self, head_ptr, ent, minmax):

        if self.verbose:
            print("_rbtree_minmax args :%s %s %d" % (head_ptr, ent, minmax))
        tmp = self._rbtree_root(head_ptr)
        parent = 0
        while tmp != 0:
            parent = tmp
            if minmax < 0:
                tmp = self._rbtree_left(tmp, ent)
            else:
                tmp = self._rbtree_right(tmp, ent)

        return parent
    def _rbtree_min(self, head_ptr, ent):
        if self.verbose:
            print("_rbtree_min args :%s %s" % (head_ptr, ent))
        return self._rbtree_minmax(head_ptr, ent, -1)
    def _rbtree_max(self, head_ptr, ent):
        if self.verbose:
            print("_rbtree_max args :%s %s" % (head_ptr, ent))
        return self._rbtree_minmax(head_ptr, ent, 1)
    def _rbtree_next(self, node_ptr, ent):
        if self.verbose:
            print("_rbtree_next args :%s %s" % (node_ptr, ent))
        elm = node_ptr
        if self._rbtree_right(elm, ent) != 0:
            elm = self._rbtree_right(elm, ent)
            while self._rbtree_left(elm, ent) != 0:
                elm = self._rbtree_left(elm, ent)
        else:
            if self._rbtree_parent(elm, ent) != 0 and elm == self._rbtree_left(self._rbtree_parent(elm, ent), ent):
                elm = self._rbtree_parent(elm, ent)
            else:
                while self._rbtree_parent(elm, ent) != 0 and elm == self._rbtree_right(self._rbtree_parent(elm, ent), ent):
                    elm = self._rbtree_parent(elm, ent)
                elm = self._rbtree_parent(elm, ent)

        return elm
    def _rbtree_prev(self, node_ptr, ent):
        if self.verbose:
            print("_rbtree_prev args :%s %s" % (node_ptr, ent))
        elm = node_ptr
        if self._rbtree_left(elm, ent) != 0:
            elm = self._rbtree_left(elm, ent)
            while self._rbtree_right(elm, ent) != 0:
                elm = self._rbtree_right(elm, ent)
        else:
            if self._rbtree_parent(elm, ent) != 0 and elm == self._rbtree_right(self._rbtree_parent(elm, ent), ent):
                elm = self._rbtree_parent(elm, ent)
            else:
                while self._rbtree_parent(elm, ent) != 0 and elm == self._rbtree_left(self._rbtree_parent(elm, ent), ent):
                    elm = self._rbtree_parent(elm, ent)
                elm = self._rbtree_parent(elm, ent)

        return elm

class ThreadPrettyPrinter(object):
    """Print struct _thread """
    def _thread_state_string(self, state):
        if state == 0:
            return "DORMANT"
        if state == 1:
            return "RUN"
        if state == 2:
            return "RUNNABLE"
        if state == 3:
            return "WAIT"
        if state == 4:
            return "EXIT"
        if state == 5:
            return "DEAD"
        return "UNKNOWN"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        res = "thread-id: %d state: %s(%d) thread-info: 0x%x ksp: 0x%x proc:0x%x" % (self.val["id"], self._thread_state_string(self.val["state"]), self.val["state"], self.val["tinfo"], self.val["ksp"], self.val["p"])
        return res

class ProcPrettyPrinter(object):
    """Print struct _proc """

    def __init__(self, val):
        self.val = val

    def to_string(self):
        res = "proc-id: %d pgt: 0x%x name:%s master:0x%x" % (self.val["id"], self.val["pgt"], self.val["name"], self.val["master"])
        return res

class ThreadPoolDumpCmd(gdb.Command):
    """Prints the Thread Pool"""

    def __init__(self):
        self.parser = RBTreeParser()
        self.verbose = False
        super(ThreadPoolDumpCmd, self).__init__("thread_dump", gdb.COMMAND_USER)

    def _value_to_string(self, val):
        res = ""
        if str(val.type) == "struct _thread *" or str(val.type) == "thread *":
            res = ("Thread: 0x%x " % val) + ThreadPrettyPrinter(val.dereference()).to_string()
        return res
    def _thread_pool_to_array(self, val, type, ent):
        """Walk through the rbtree.
        """
        if self.verbose:
            print("arg=%s %s %s" % (val, type, ent))
        idx = 0
        head_ptr = val
        result = []
        node_ptr = self.parser._rbtree_root(head_ptr)
        if self.verbose:
            print("root-node:%s" % node_ptr)
        if node_ptr == 0:
            print("Thread pool is empty.")
            return "" #木が空
        min_node = self.parser._rbtree_min(head_ptr, ent)
        max_node = self.parser._rbtree_max(head_ptr, ent)
        if self.verbose:
            print("min-node:%s max-node:%s" % (min_node, max_node))
        cur_node = min_node
        while cur_node != 0:
            if self.verbose:
                print("cur:%s" % cur_node)
            result.append(self._value_to_string(cur_node))
            cur_node = self.parser._rbtree_next(cur_node, ent)
        return result

    def complete(self, text, word):
        # We expect the argument passed to be a symbol so fallback to the
        # internal tab-completion handler for symbols
        return gdb.COMPLETE_SYMBOL

    def invoke(self, args, from_tty):
        # We can pass args here and use Python CLI utilities like argparse
        # to do argument parsing
        if self.verbose:
            print("Args Passed: %s" % args)
        node_ptr_val = gdb.parse_and_eval("&g_thrdb.head")
        if str(node_ptr_val.type) != "struct _thrdb_tree *"  and str(node_ptr_val.type) != "thrdb_tree *":
            print("Expected pointer argument of type (struct _thrdb_tree *)")
            print("node: %s" % str(node_ptr_val.type))
            return
        for res in self._thread_pool_to_array(node_ptr_val, "_thrdb_tree", "ent"):
            print("%s" % res)

class ProcPoolDumpCmd(gdb.Command):
    """Prints the Process Pool"""

    def __init__(self):
        self.parser = RBTreeParser()
        self.verbose = False
        super(ProcPoolDumpCmd, self).__init__("proc_dump", gdb.COMMAND_USER)

    def _value_to_string(self, val):
        res = ""
        if str(val.type) == "struct _proc *" or str(val.type) == "proc *":
            res =  ("Proc: 0x%x " % val) + ProcPrettyPrinter(val.dereference()).to_string()
        return res
    def _proc_pool_to_array(self, val, type, ent):
        """Walk through the rbtree.
        """
        if self.verbose:
            print("arg=%s %s %s" % (val, type, ent))
        idx = 0
        head_ptr = val
        result = []
        node_ptr = self.parser._rbtree_root(head_ptr)
        if self.verbose:
            print("root-node:%s" % node_ptr)
        if node_ptr == 0:
            print("Proc pool is empty.")
            return "" #木が空
        min_node = self.parser._rbtree_min(head_ptr, ent)
        max_node = self.parser._rbtree_max(head_ptr, ent)
        if self.verbose:
            print("min-node:%s max-node:%s" % (min_node, max_node))
        cur_node = min_node
        while cur_node != 0:
            if self.verbose:
                print("cur:%s" % cur_node)
            result.append(self._value_to_string(cur_node))
            cur_node = self.parser._rbtree_next(cur_node, ent)
        return result

    def complete(self, text, word):
        # We expect the argument passed to be a symbol so fallback to the
        # internal tab-completion handler for symbols
        return gdb.COMPLETE_SYMBOL

    def invoke(self, args, from_tty):
        # We can pass args here and use Python CLI utilities like argparse
        # to do argument parsing
        if self.verbose:
            print("Args Passed: %s" % args)
        node_ptr_val = gdb.parse_and_eval("&g_procdb.head")
        if str(node_ptr_val.type) != "struct _procdb_tree *"  and str(node_ptr_val.type) != "procdb_tree *":
            print("Expected pointer argument of type (struct _procdb_tree *)")
            print("node: %s" % str(node_ptr_val.type))
            return
        for res in self._proc_pool_to_array(node_ptr_val, "_procdb_tree", "ent"):
            print("%s" % res)

class CustomPrettyPrinterLocator(PrettyPrinter):
        """Given a gdb.Value, search for a custom pretty printer"""

        def __init__(self):
            super(CustomPrettyPrinterLocator, self).__init__(
                "my_pretty_printers", []
            )

        def __call__(self, val):
            """Return the custom formatter if the type can be handled"""

            typename = gdb.types.get_basic_type(val.type).tag
            #print("Typename:%s" % typename)
            if typename is None:
                typename = val.type.name

#            if typename == "_thread":
#                return ThreadPrettyPrinter(val)


register_pretty_printer(None, CustomPrettyPrinterLocator(), replace=True)
ThreadPoolDumpCmd()
ProcPoolDumpCmd()
