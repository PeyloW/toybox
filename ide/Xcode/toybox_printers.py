"""
LLDB formatters for toybox C++ types.
"""

import lldb
import re


def __lldb_init_module(debugger, internal_dict):
    """Register type summaries when module is loaded."""
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?base_fix_t<.+>$" -F toybox_printers.base_fix_t_summary'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?base_point_s<.+>$" -F toybox_printers.base_point_s_summary'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?base_size_s<.+>$" -F toybox_printers.base_size_s_summary'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?base_rect_s<.+>$" -F toybox_printers.base_rect_s_summary'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?cc4_t$" -F toybox_printers.cc4_t_summary'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?color_c$" -F toybox_printers.color_c_summary'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?iff_chunk_s$" -F toybox_printers.iff_chunk_s_summary'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?iff_group_s$" -F toybox_printers.iff_group_s_summary'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?unique_ptr_c<.+>$" -F toybox_printers.unique_ptr_c_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?unique_ptr_c<.+>$" -l toybox_printers.SmartPtrSyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?shared_ptr_c<.+>$" -F toybox_printers.shared_ptr_c_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?shared_ptr_c<.+>$" -l toybox_printers.SmartPtrSyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?array_s<.+>$" -F toybox_printers.array_s_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?array_s<.+>$" -l toybox_printers.ArraySyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?basic_palette_c<.+>$" -F toybox_printers.array_s_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?basic_palette_c<.+>$" -l toybox_printers.ArraySyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?palette_c$" -F toybox_printers.array_s_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?palette_c$" -l toybox_printers.ArraySyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?vector_c<.+>$" -F toybox_printers.vector_c_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?vector_c<.+>$" -l toybox_printers.VectorSyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?map_c<.+>$" -F toybox_printers.map_c_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?map_c<.+>$" -l toybox_printers.MapSyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?pair_c<.+>$" -F toybox_printers.pair_c_summary'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?list_c<.+>$" -F toybox_printers.list_c_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?list_c<.+>$" -l toybox_printers.ListSyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?display_list_c$" -F toybox_printers.list_c_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?display_list_c$" -l toybox_printers.ListSyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?list_c<.+>::detail::iterator_s<.+>$" -F toybox_printers.list_iterator_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?list_c<.+>::detail::iterator_s<.+>$" -l toybox_printers.ListIteratorSyntheticProvider'
    )
    debugger.HandleCommand(
        'type summary add -x "^(toybox::)?expected_c<.+>$" -F toybox_printers.expected_c_summary'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^(toybox::)?expected_c<.+>$" -l toybox_printers.ExpectedSyntheticProvider'
    )
    print("Toybox LLDB formatters loaded.")


# =============================================================================
# Base classes for synthetic providers
# =============================================================================

class IndexedSyntheticProvider:
    """Base class for synthetic providers with indexed children like [0], [1], etc."""

    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.children = []

    def num_children(self):
        return len(self.children)

    def get_child_index(self, name):
        try:
            if name.startswith('[') and name.endswith(']'):
                return int(name[1:-1])
            return int(name)
        except ValueError:
            return -1

    def get_child_at_index(self, index):
        if 0 <= index < len(self.children):
            return self.children[index]
        return None

    def has_children(self):
        return len(self.children) > 0

    def update(self):
        """Subclasses must implement this to populate self.children."""
        raise NotImplementedError


class NamedSyntheticProvider:
    """Base class for synthetic providers with named children."""

    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.children = []

    def num_children(self):
        return len(self.children)

    def get_child_index(self, name):
        for i, child in enumerate(self.children):
            if child.GetName() == name:
                return i
        return -1

    def get_child_at_index(self, index):
        if 0 <= index < len(self.children):
            return self.children[index]
        return None

    def has_children(self):
        return len(self.children) > 0

    def update(self):
        """Subclasses must implement this to populate self.children."""
        raise NotImplementedError


# =============================================================================
# Helper functions
# =============================================================================

def get_member_recursive(valobj, member_name):
    """Get a member by name, searching through base classes if needed."""
    member = valobj.GetChildMemberWithName(member_name)
    if member.IsValid():
        return member
    for i in range(valobj.GetNumChildren()):
        child = valobj.GetChildAtIndex(i)
        if child.IsValid():
            member = child.GetChildMemberWithName(member_name)
            if member.IsValid():
                return member
    return None


def collect_children(valobj):
    """Collect all children of a value object into a list."""
    children = []
    if valobj and valobj.IsValid():
        for i in range(valobj.GetNumChildren()):
            child = valobj.GetChildAtIndex(i)
            if child.IsValid():
                children.append(child)
    return children


def get_cc4_string(valobj):
    """Extract string from cc4_t value object."""
    ubytes = valobj.GetChildMemberWithName('ubytes')
    if not ubytes.IsValid():
        return None
    chars = []
    for i in range(4):
        byte = ubytes.GetChildAtIndex(i).GetValueAsUnsigned()
        if 32 <= byte <= 126:
            chars.append(chr(byte))
        else:
            chars.append('?')
    return ''.join(chars)


# =============================================================================
# Summary functions
# =============================================================================

def base_fix_t_summary(valobj, internal_dict):
    """Format base_fix_t<Int, Bits> as floating point value."""
    try:
        raw_member = valobj.GetChildMemberWithName('raw')
        if not raw_member.IsValid():
            return "<invalid>"

        raw_value = raw_member.GetValueAsSigned()
        type_name = valobj.GetType().GetName()

        match = re.search(r'base_fix_t<[^,]+,\s*(\d+)>', type_name)
        if match:
            frac_bits = int(match.group(1))
        elif 'fix16_t' in type_name:
            frac_bits = 4
        else:
            return "<unknown format>"

        float_value = raw_value / (1 << frac_bits)
        return f"{float_value}"

    except Exception as e:
        return f"<error: {e}>"


def base_point_s_summary(valobj, internal_dict):
    """Format base_point_s<Type> as {x, y}."""
    try:
        x = valobj.GetChildMemberWithName('x')
        y = valobj.GetChildMemberWithName('y')
        if not x.IsValid() or not y.IsValid():
            return "<invalid>"
        x_summary = x.GetSummary() or x.GetValue()
        y_summary = y.GetSummary() or y.GetValue()
        return f"{{{x_summary}, {y_summary}}}"
    except Exception as e:
        return f"<error: {e}>"


def base_size_s_summary(valobj, internal_dict):
    """Format base_size_s<Type> as {width, height}."""
    try:
        w = valobj.GetChildMemberWithName('width')
        h = valobj.GetChildMemberWithName('height')
        if not w.IsValid() or not h.IsValid():
            return "<invalid>"
        w_summary = w.GetSummary() or w.GetValue()
        h_summary = h.GetSummary() or h.GetValue()
        return f"{{{w_summary}, {h_summary}}}"
    except Exception as e:
        return f"<error: {e}>"


def base_rect_s_summary(valobj, internal_dict):
    """Format base_rect_s<Type> as {origin, size}."""
    try:
        origin = valobj.GetChildMemberWithName('origin')
        size = valobj.GetChildMemberWithName('size')
        if not origin.IsValid() or not size.IsValid():
            return "<invalid>"
        origin_summary = origin.GetSummary() or "<invalid>"
        size_summary = size.GetSummary() or "<invalid>"
        return f"{{{origin_summary}, {size_summary}}}"
    except Exception as e:
        return f"<error: {e}>"


def cc4_t_summary(valobj, internal_dict):
    """Format cc4_t as [ABCD]."""
    try:
        cc4_str = get_cc4_string(valobj)
        if cc4_str:
            return f"[{cc4_str}]"
        return "<invalid>"
    except Exception as e:
        return f"<error: {e}>"


def color_c_summary(valobj, internal_dict):
    """Format color_c as #RGB with STe color correction."""
    try:
        color = valobj.GetChildMemberWithName('color')
        if not color.IsValid():
            return "<invalid>"
        c = color.GetValueAsUnsigned()
        # STe stores nibbles with bit 3 as LSB
        STE_FROM_SEQ = [0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15]
        r = STE_FROM_SEQ[(c >> 8) & 0x0f]
        g = STE_FROM_SEQ[(c >> 4) & 0x0f]
        b = STE_FROM_SEQ[c & 0x0f]
        return f"#{r:X}{g:X}{b:X}"
    except Exception as e:
        return f"<error: {e}>"


def iff_chunk_s_summary(valobj, internal_dict):
    """Format iff_chunk_s as [ID] size."""
    try:
        id_member = valobj.GetChildMemberWithName('id')
        size_member = valobj.GetChildMemberWithName('size')
        if not id_member.IsValid() or not size_member.IsValid():
            return "<invalid>"
        id_str = get_cc4_string(id_member) or "????"
        size = size_member.GetValueAsUnsigned()
        return f"[{id_str}] {size}"
    except Exception as e:
        return f"<error: {e}>"


def iff_group_s_summary(valobj, internal_dict):
    """Format iff_group_s as [ID:SUBTYPE] size."""
    try:
        id_member = valobj.GetChildMemberWithName('id')
        subtype_member = valobj.GetChildMemberWithName('subtype')
        size_member = valobj.GetChildMemberWithName('size')
        if not id_member.IsValid() or not subtype_member.IsValid() or not size_member.IsValid():
            return "<invalid>"
        id_str = get_cc4_string(id_member) or "????"
        subtype_str = get_cc4_string(subtype_member) or "????"
        size = size_member.GetValueAsUnsigned()
        return f"[{id_str}:{subtype_str}] {size}"
    except Exception as e:
        return f"<error: {e}>"


# =============================================================================
# Smart pointer support
# =============================================================================

def get_ptr_value(valobj):
    """Get _ptr from unique_ptr_c or shared_ptr_c."""
    valobj_non_synth = valobj.GetNonSyntheticValue()

    # Try direct access
    ptr = valobj_non_synth.GetChildMemberWithName('_ptr')
    if ptr and ptr.IsValid():
        return ptr

    # Search through all children recursively (handles multiple inheritance)
    def search_for_ptr(obj, depth=0):
        if depth > 3:
            return None
        for i in range(obj.GetNumChildren()):
            child = obj.GetChildAtIndex(i)
            if not child.IsValid():
                continue
            # Check if this child is _ptr
            if child.GetName() == '_ptr':
                return child
            # Check if child has _ptr
            ptr = child.GetChildMemberWithName('_ptr')
            if ptr and ptr.IsValid():
                return ptr
            # Recurse into child
            result = search_for_ptr(child, depth + 1)
            if result:
                return result
        return None

    return search_for_ptr(valobj_non_synth)


def get_count_value(valobj):
    """Get _count from shared_ptr_c."""
    valobj_non_synth = valobj.GetNonSyntheticValue()
    return get_member_recursive(valobj_non_synth, '_count')


def unique_ptr_c_summary(valobj, internal_dict):
    """Format unique_ptr_c as 0xADDRESS (u)."""
    try:
        ptr = get_ptr_value(valobj)
        if not ptr:
            return "<invalid>"
        addr = ptr.GetValueAsUnsigned()
        if addr == 0:
            return "nullptr"
        return f"0x{addr:x} (u)"
    except Exception as e:
        return f"<error: {e}>"


def shared_ptr_c_summary(valobj, internal_dict):
    """Format shared_ptr_c as 0xADDRESS (s=N)."""
    try:
        ptr = get_ptr_value(valobj)
        if not ptr:
            return "<invalid ptr>"
        addr = ptr.GetValueAsUnsigned()
        if addr == 0:
            return "nullptr"
        count_ptr = get_count_value(valobj)
        if count_ptr and count_ptr.IsValid() and count_ptr.GetValueAsUnsigned() != 0:
            count_obj = count_ptr.Dereference()
            count = count_obj.GetChildMemberWithName('count').GetValueAsUnsigned()
        else:
            count = 0
        return f"0x{addr:x} (s={count})"
    except Exception as e:
        return f"<error: {e}>"


class SmartPtrSyntheticProvider(NamedSyntheticProvider):
    """Synthetic children provider for unique_ptr_c and shared_ptr_c."""

    def update(self):
        self.children = []
        ptr = get_ptr_value(self.valobj)
        if not ptr or not ptr.IsValid():
            return
        # Ensure ptr is actually a pointer type
        if not ptr.GetType().IsPointerType():
            return
        addr = ptr.GetValueAsUnsigned()
        if addr == 0:
            return
        pointee = ptr.Dereference()
        if not pointee.IsValid():
            return
        children = collect_children(pointee)
        if children:
            self.children = children
        else:
            # For primitive types (no children), show the pointee itself
            self.children = [pointee.CreateChildAtOffset(
                '*', 0, pointee.GetType()
            )]

    def has_children(self):
        ptr = get_ptr_value(self.valobj)
        if ptr and ptr.IsValid():
            if ptr.GetType().IsPointerType() and ptr.GetValueAsUnsigned() != 0:
                return True
        return False


# =============================================================================
# Array support
# =============================================================================

def array_s_summary(valobj, internal_dict):
    """Format array_s<Type, Count> as size indicator."""
    try:
        valobj_non_synth = valobj.GetNonSyntheticValue()
        data = valobj_non_synth.GetChildMemberWithName('_data')
        if data.IsValid():
            count = data.GetNumChildren()
            return f"size={count}"
        return "<invalid>"
    except Exception as e:
        return f"<error: {e}>"


class ArraySyntheticProvider(IndexedSyntheticProvider):
    """Synthetic children provider for array_s - shows _data elements directly."""

    def update(self):
        self.children = []
        valobj_non_synth = self.valobj.GetNonSyntheticValue()
        data = valobj_non_synth.GetChildMemberWithName('_data')
        if data and data.IsValid():
            self.children = collect_children(data)


# =============================================================================
# Vector support
# =============================================================================

def vector_c_summary(valobj, internal_dict):
    """Format vector_c as size=N."""
    try:
        valobj_non_synth = valobj.GetNonSyntheticValue()
        size = valobj_non_synth.GetChildMemberWithName('_size')
        if size and size.IsValid():
            return f"size={size.GetValueAsUnsigned()}"
        return "<invalid>"
    except Exception as e:
        return f"<error: {e}>"


class VectorSyntheticProvider(IndexedSyntheticProvider):
    """Synthetic children provider for vector_c."""

    def update(self):
        self.children = []
        valobj_non_synth = self.valobj.GetNonSyntheticValue()

        size_val = valobj_non_synth.GetChildMemberWithName('_size')
        if not size_val or not size_val.IsValid():
            return
        size = size_val.GetValueAsUnsigned()

        buffer = get_member_recursive(valobj_non_synth, '_buffer')
        if not buffer or not buffer.IsValid():
            return

        buffer_type = buffer.GetType()
        if buffer_type.IsPointerType():
            membuf_type = buffer_type.GetPointeeType()
            buffer_addr = buffer.GetValueAsUnsigned()
        elif buffer_type.IsArrayType():
            membuf_type = buffer_type.GetArrayElementType()
            buffer_addr = buffer.GetLoadAddress()
        else:
            return

        if buffer_addr == 0 or buffer_addr == 0xffffffffffffffff:
            return

        membuf_size = membuf_type.GetByteSize()
        num_template_args = membuf_type.GetNumberOfTemplateArguments()
        if num_template_args == 0:
            return
        element_type = membuf_type.GetTemplateArgumentType(0)
        if not element_type or not element_type.IsValid():
            return

        for i in range(size):
            addr = buffer_addr + i * membuf_size
            child = self.valobj.CreateValueFromAddress(f"[{i}]", addr, element_type)
            if child.IsValid():
                self.children.append(child)


# =============================================================================
# Pair support
# =============================================================================

def get_value_display(valobj):
    """Get a display string for a value, or None if it should be expanded."""
    if not valobj or not valobj.IsValid():
        return None
    # Try summary first (for types with custom formatters)
    summary = valobj.GetSummary()
    if summary:
        return summary
    # For simple types, use GetValue()
    value = valobj.GetValue()
    if value:
        return value
    # Complex types without summary - don't try to display inline
    return None


def pair_c_summary(valobj, internal_dict):
    """Format pair_c as {first, second} for simple types, or {first, ...} for complex."""
    try:
        first = valobj.GetChildMemberWithName('first')
        second = valobj.GetChildMemberWithName('second')
        if not first.IsValid() or not second.IsValid():
            return "<invalid>"
        first_str = get_value_display(first)
        second_str = get_value_display(second)
        # Show key if available, indicate expansion needed for complex values
        if first_str and second_str:
            return f"{{{first_str}: {second_str}}}"
        elif first_str:
            return f"{{{first_str}: ...}}"
        else:
            return "{...}"
    except Exception as e:
        return f"<error: {e}>"


# =============================================================================
# Map support
# =============================================================================

def map_c_summary(valobj, internal_dict):
    """Format map_c as size=N."""
    try:
        valobj_non_synth = valobj.GetNonSyntheticValue()
        size = valobj_non_synth.GetChildMemberWithName('_size')
        if size and size.IsValid():
            return f"size={size.GetValueAsUnsigned()}"
        return "<invalid>"
    except Exception as e:
        return f"<error: {e}>"


class MapSyntheticProvider(IndexedSyntheticProvider):
    """Synthetic children provider for map_c."""

    def update(self):
        self.children = []
        valobj_non_synth = self.valobj.GetNonSyntheticValue()

        size_val = valobj_non_synth.GetChildMemberWithName('_size')
        if not size_val or not size_val.IsValid():
            return
        size = size_val.GetValueAsUnsigned()

        buffer = get_member_recursive(valobj_non_synth, '_buffer')
        if not buffer or not buffer.IsValid():
            return

        buffer_type = buffer.GetType()
        if buffer_type.IsPointerType():
            membuf_type = buffer_type.GetPointeeType()
            buffer_addr = buffer.GetValueAsUnsigned()
        elif buffer_type.IsArrayType():
            membuf_type = buffer_type.GetArrayElementType()
            buffer_addr = buffer.GetLoadAddress()
        else:
            return

        if buffer_addr == 0 or buffer_addr == 0xffffffffffffffff:
            return

        membuf_size = membuf_type.GetByteSize()
        num_template_args = membuf_type.GetNumberOfTemplateArguments()
        if num_template_args == 0:
            return
        element_type = membuf_type.GetTemplateArgumentType(0)
        if not element_type or not element_type.IsValid():
            return

        for i in range(size):
            addr = buffer_addr + i * membuf_size
            child = self.valobj.CreateValueFromAddress(f"[{i}]", addr, element_type)
            if child.IsValid():
                self.children.append(child)


# =============================================================================
# List support
# =============================================================================

def get_list_head(valobj):
    """Get _head from list_c."""
    valobj_non_synth = valobj.GetNonSyntheticValue()
    head = valobj_non_synth.GetChildMemberWithName('_head')
    if head and head.IsValid():
        return head
    return get_member_recursive(valobj_non_synth, '_head')


def list_c_summary(valobj, internal_dict):
    """Format list_c as size=N."""
    try:
        head = get_list_head(valobj)
        if not head or not head.IsValid():
            return "<invalid>"
        count = 0
        node_addr = head.GetValueAsUnsigned()
        visited = set()
        current = head
        while node_addr != 0 and node_addr not in visited:
            visited.add(node_addr)
            count += 1
            node = current.Dereference()
            next_ptr = node.GetChildMemberWithName('next')
            if not next_ptr.IsValid():
                break
            node_addr = next_ptr.GetValueAsUnsigned()
            current = next_ptr
        return f"size={count}"
    except Exception as e:
        return f"<error: {e}>"


class ListSyntheticProvider(IndexedSyntheticProvider):
    """Synthetic children provider for list_c."""

    def update(self):
        self.children = []
        head = get_list_head(self.valobj)
        if not head or not head.IsValid():
            return
        node_addr = head.GetValueAsUnsigned()
        visited = set()
        index = 0
        current = head
        while node_addr != 0 and node_addr not in visited:
            visited.add(node_addr)
            node = current.Dereference()
            if not node.IsValid():
                break
            value = node.GetChildMemberWithName('value')
            if value.IsValid():
                self.children.append(
                    value.CreateChildAtOffset(f"[{index}]", 0, value.GetType())
                )
            index += 1
            next_ptr = node.GetChildMemberWithName('next')
            if not next_ptr.IsValid():
                break
            node_addr = next_ptr.GetValueAsUnsigned()
            current = next_ptr


# =============================================================================
# List iterator support
# =============================================================================

def get_list_iterator_value(valobj):
    """Get the value pointer from list iterator."""
    valobj_non_synth = valobj.GetNonSyntheticValue()
    node = valobj_non_synth.GetChildMemberWithName('_node')
    if not node or not node.IsValid():
        return None, None
    node_addr = node.GetValueAsUnsigned()
    if node_addr == 0:
        return None, None
    node_deref = node.Dereference()
    if not node_deref.IsValid():
        return None, None
    value = node_deref.GetChildMemberWithName('value')
    return value, node_addr


def list_iterator_summary(valobj, internal_dict):
    """Format list iterator as pointer to value."""
    try:
        value, node_addr = get_list_iterator_value(valobj)
        if not value or not value.IsValid():
            if node_addr == 0 or node_addr is None:
                return "nullptr"
            return "<invalid>"
        value_addr = value.GetLoadAddress()
        return f"0x{value_addr:x}"
    except Exception as e:
        return f"<error: {e}>"


class ListIteratorSyntheticProvider(NamedSyntheticProvider):
    """Synthetic children provider for list iterator - shows value's members directly."""

    def update(self):
        self.children = []
        value, _ = get_list_iterator_value(self.valobj)
        self.children = collect_children(value)


# =============================================================================
# Expected support
# =============================================================================

def expected_c_summary(valobj, internal_dict):
    """Format expected_c as error=N or show value summary."""
    try:
        valobj_non_synth = valobj.GetNonSyntheticValue()
        error = valobj_non_synth.GetChildMemberWithName('_error')
        if not error or not error.IsValid():
            return "<invalid>"
        error_val = error.GetValueAsSigned()
        if error_val != 0:
            return f"error={error_val}"
        value = valobj_non_synth.GetChildMemberWithName('_value')
        if value and value.IsValid():
            summary = value.GetSummary()
            if summary:
                return summary
            # Handle pointer types without leading zeros
            if value.GetType().IsPointerType():
                addr = value.GetValueAsUnsigned()
                if addr == 0:
                    return "nullptr"
                return f"0x{addr:x}"
            val_str = value.GetValue()
            if val_str:
                return val_str
        return "<valid>"
    except Exception as e:
        return f"<error: {e}>"


class ExpectedSyntheticProvider(NamedSyntheticProvider):
    """Synthetic children provider for expected_c - shows value's children when no error."""

    def update(self):
        self.children = []
        valobj_non_synth = self.valobj.GetNonSyntheticValue()
        error = valobj_non_synth.GetChildMemberWithName('_error')
        if not error or not error.IsValid():
            return
        if error.GetValueAsSigned() != 0:
            return
        value = valobj_non_synth.GetChildMemberWithName('_value')
        if not value or not value.IsValid():
            return
        children = collect_children(value)
        if children:
            self.children = children
        else:
            # For primitive types, show the value itself
            self.children = [value.CreateChildAtOffset(
                '*', 0, value.GetType()
            )]

    def has_children(self):
        valobj_non_synth = self.valobj.GetNonSyntheticValue()
        error = valobj_non_synth.GetChildMemberWithName('_error')
        if error and error.IsValid() and error.GetValueAsSigned() == 0:
            return True
        return False
