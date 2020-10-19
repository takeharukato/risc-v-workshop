# -*- coding: utf-8 mode: python -*-
# gdb pythonスクリプト
# 参考: https://interrupt.memfault.com/blog/automate-debugging-with-gdb-python-api
#
from gdb.printing import PrettyPrinter, register_pretty_printer
import gdb

class RBTreeDumpCmd(gdb.Command):
    """Prints the RBTree"""

    def __init__(self):
        super(RBTreeDumpCmd, self).__init__("rbtree_dump", gdb.COMMAND_USER)
    def _rbtree_root(self, val):
        node_ptr = val
        return node_ptr["rbh_root"]
    def _rbtree_parent(self, val, ent):
        node_ptr = val
#        print("_rbtree_parent args :%s %s" % (val, ent))
        tree_ent = node_ptr[ent]
        tree_left = tree_ent["rbe_parent"]
        return tree_left
    def _rbtree_left(self, val, ent):
        node_ptr = val
#        print("_rbtree_left args :%s %s" % (val, ent))
        tree_ent = node_ptr[ent]
        tree_left = tree_ent["rbe_left"]
        return tree_left
    def _rbtree_right(self, val, ent):
        node_ptr = val
#        print("_rbtree_right args :%s %s" % (val, ent))
        tree_ent = node_ptr[ent]
        tree_right = tree_ent["rbe_right"]
        return tree_right

    def _rbtree_minmax(self, head_ptr, ent, minmax):

#        print("_rbtree_minmax args :%s %s %d" % (head_ptr, ent, minmax))

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
#        print("_rbtree_min args :%s %s" % (head_ptr, ent))
        return self._rbtree_minmax(head_ptr, ent, -1)
    def _rbtree_max(self, head_ptr, ent):
#        print("_rbtree_max args :%s %s" % (head_ptr, ent))
        return self._rbtree_minmax(head_ptr, ent, 1)
    def _rbtree_next(self, node_ptr, ent):

#        print("_rbtree_next args :%s %s" % (node_ptr, ent))
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

    def _value_to_string(self, val):
        if str(val.type) == "struct _thread *" or str(val.type) == "thread *":
            res = "thread: 0x%x thread-id: %d state: %s(%d) thread-info: 0x%x ksp: 0x%x proc:0x%x" % (val , val["id"], self._thread_state_string(val["state"]), val["state"], val["tinfo"], val["ksp"], val["p"])
        return res
    def _rbtree_to_array(self, val, type, ent):
        """Walk through the rbtree.
        We will simply follow the 'next' pointers until we encounter NULL
        """
#        print("arg=%s %s %s" % (val, type, ent))
        idx = 0
        head_ptr = val
        result = []
        node_ptr = self._rbtree_root(head_ptr)
#        print("root-node:%s" % node_ptr)
        if node_ptr == 0:
            return "" #木が空
        left_node = self._rbtree_left(node_ptr, ent)
        right_node = self._rbtree_right(node_ptr, ent)
#        print("left-node:%s right-node:%s" % (left_node, right_node))
        min_node = self._rbtree_min(head_ptr, ent)
        max_node = self._rbtree_max(head_ptr, ent)
#        print("min-node:%s max-node:%s" % (min_node, max_node))
#        print("next: %s" % self._rbtree_next(min_node, ent))
        cur_node = self._rbtree_minmax(head_ptr, ent, -1)
        while cur_node != 0:
#            print("cur:%s" % cur_node)
            result.append(self._value_to_string(cur_node))
            cur_node = self._rbtree_next(cur_node, ent)
        return result

    def complete(self, text, word):
        # We expect the argument passed to be a symbol so fallback to the
        # internal tab-completion handler for symbols
        return gdb.COMPLETE_SYMBOL

    def invoke(self, args, from_tty):
        # We can pass args here and use Python CLI utilities like argparse
        # to do argument parsing
#        print("Args Passed: %s" % args)
        arg_array = args.split()
#        for arg in arg_array:
#            print("arg:%s" % arg)
        node_ptr_val = gdb.parse_and_eval(arg_array[0])
        if str(node_ptr_val.type) != "struct %s *" % arg_array[1] and str(node_ptr_val.type) != "%s *" % arg_array[1]:
            print("Expected pointer argument of type (%s *)" % arg_array[1])
            print("node: %s" % str(node_ptr_val.type))
            return
        for res in self._rbtree_to_array(node_ptr_val, arg_array[1], arg_array[2]):
            print("%s" % res)

#使用例
#rbtree_dump &g_thrdb.head _thrdb_tree ent
#
RBTreeDumpCmd()
