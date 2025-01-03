function update_instances(changeset, orig_component, new_component)
    local instances = slc.component(orig_component).instances
    for instance,_ in pairs(instances) do
        table.insert(changeset, {
          ['component'] = orig_component,
          ['instance'] = {instance},
          ['action'] = 'remove'
        })
        table.insert(changeset, {
          ['component'] = new_component,
          ['instance'] = {instance},
          ['action'] = 'add'
        })
    end
end

local changeset = {}

-- Only update instances if application uses OT platform abstraction
if slc.is_selected('ot_platform_abstraction_core') then
    update_instances(changeset, 'uartdrv_usart', 'iostream_usart')
    update_instances(changeset, 'uartdrv_eusart', 'iostream_eusart')
end

return changeset
