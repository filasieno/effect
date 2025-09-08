(function(){
  function addButtons(){
    document.querySelectorAll('.fragment').forEach(function(block){
      if(block.querySelector('.copy-btn')) return;
      var btn = document.createElement('button');
      btn.textContent = 'Copy';
      btn.className = 'copy-btn';
      btn.style.float = 'right';
      btn.addEventListener('click', function(){
        var text = block.innerText;
        navigator.clipboard && navigator.clipboard.writeText(text);
      });
      block.prepend(btn);
    });
  }
  document.addEventListener('DOMContentLoaded', addButtons);
})();
