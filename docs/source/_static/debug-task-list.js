// 调试任务列表功能
document.addEventListener('DOMContentLoaded', function () {
    console.log('=== 任务列表调试信息 ===');

    // 1. 检查页面中的所有复选框
    const allInputs = document.querySelectorAll('input[type="checkbox"]');
    console.log('总复选框数量:', allInputs.length);

    allInputs.forEach((input, index) => {
        console.log(`复选框 ${index}:`, {
            className: input.className,
            type: input.type,
            checked: input.checked,
            disabled: input.disabled,
            style: window.getComputedStyle(input).cursor
        });
    });

    // 2. 检查任务列表项
    const taskItems = document.querySelectorAll('li');
    const taskListItems = Array.from(taskItems).filter(li => {
        const checkbox = li.querySelector('input[type="checkbox"]');
        return checkbox !== null;
    });

    console.log('任务列表项数量:', taskListItems.length);

    taskListItems.forEach((item, index) => {
        const checkbox = item.querySelector('input[type="checkbox"]');
        console.log(`任务项 ${index}:`, {
            itemClasses: item.className,
            checkboxClasses: checkbox ? checkbox.className : 'no checkbox',
            hasEventListener: checkbox ? checkbox.onclick !== null : false,
            text: item.textContent.substring(0, 50) + '...'
        });
    });

    // 3. 尝试手动绑定事件
    taskListItems.forEach((item, index) => {
        const checkbox = item.querySelector('input[type="checkbox"]');
        if (checkbox) {
            console.log(`为任务 ${index} 绑定点击事件`);

            checkbox.addEventListener('click', function (e) {
                console.log('复选框被点击:', this.checked);
                e.stopPropagation();
            });

            checkbox.addEventListener('change', function (e) {
                console.log('复选框状态改变:', this.checked);

                if (this.checked) {
                    item.style.textDecoration = 'line-through';
                    item.style.opacity = '0.7';
                } else {
                    item.style.textDecoration = 'none';
                    item.style.opacity = '1';
                }
            });
        }
    });

    // 4. 检查是否有其他脚本干扰
    console.log('页面中的所有脚本:', document.scripts.length);
    for (let i = 0; i < document.scripts.length; i++) {
        const script = document.scripts[i];
        console.log(`脚本 ${i}:`, script.src || 'inline script');
    }

    console.log('=== 调试信息结束 ===');
});
