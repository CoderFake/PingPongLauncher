(function () {
  var $ = function (id) { return document.getElementById(id); };
  var ERRORS = {
    no_ssid: 'Không thấy mạng này. Kiểm tra lại tên mạng, hoặc mạng là 5GHz (máy chỉ dùng 2.4GHz).',
    auth_failed: 'Sai mật khẩu, hoặc router từ chối kết nối.',
    weak_security: 'Router dùng kiểu bảo mật máy không hỗ trợ. Đổi router sang WPA2-PSK (AES) rồi thử lại.',
    timeout: 'Hết thời gian chờ. Đặt máy gần router hơn rồi thử lại.'
  };
  var ICON_LOCK = '<svg viewBox="0 0 24 24"><rect x="5" y="11" width="14" height="10" rx="2"/><path d="M8 11V8a4 4 0 0 1 8 0v3"/></svg>';
  var ICON_NEXT = '<svg viewBox="0 0 24 24"><path d="M9 6l6 6-6 6"/></svg>';
  var views = ['pickView', 'joinView', 'waitView', 'doneView'];
  var selected = null;
  var pollTimer = null;
  var lastReplyAt = 0;
  var apName = '';

  function show(view) {
    views.forEach(function (v) { $(v).hidden = v !== view; });
    window.scrollTo(0, 0);
  }

  function esc(text) {
    return String(text).replace(/[&<>"']/g, function (c) {
      return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c];
    });
  }

  function bars(rssi) {
    var level = rssi > -55 ? 4 : rssi > -67 ? 3 : rssi > -78 ? 2 : 1;
    var html = '<span class="bars" aria-hidden="true">';
    for (var i = 1; i <= 4; i++) html += '<i class="' + (i <= level ? 'on' : '') + '"></i>';
    return html + '</span>';
  }

  function renderList(networks) {
    var list = $('list');
    if (!networks.length) {
      list.innerHTML = '<li class="empty">Không thấy mạng nào gần đây.</li>';
      return;
    }
    networks.sort(function (a, b) { return b.rssi - a.rssi; });
    list.innerHTML = networks.map(function (n, i) {
      return '<li><button class="net" type="button" data-i="' + i + '">' + bars(n.rssi) +
        '<span class="name">' + esc(n.ssid) + '</span>' +
        (n.secure ? '<span class="lock" aria-label="Có mật khẩu">' + ICON_LOCK + '</span>' : '') +
        '<span class="go">' + ICON_NEXT + '</span></button></li>';
    }).join('');
    Array.prototype.forEach.call(list.querySelectorAll('.net'), function (btn) {
      btn.onclick = function () { openJoin(networks[btn.dataset.i]); };
    });
  }

  function scan(refresh) {
    $('rescan').classList.add('busy');
    $('scanLabel').textContent = 'Đang tìm mạng…';
    fetch('/scan' + (refresh ? '?refresh=1' : ''))
      .then(function (r) { return r.json(); })
      .then(function (d) {
        if (d.scanning) { setTimeout(function () { scan(false); }, 1200); return; }
        $('rescan').classList.remove('busy');
        $('scanLabel').textContent = d.networks.length + ' mạng xung quanh';
        renderList(d.networks);
      })
      .catch(function () { setTimeout(function () { scan(false); }, 2000); });
  }

  function openJoin(network) {
    selected = network;
    var hidden = !network;
    $('joinTitle').textContent = hidden ? 'Mạng ẩn' : network.ssid;
    $('joinLead').textContent = hidden ? 'Nhập đúng tên mạng và mật khẩu.'
      : network.secure ? 'Nhập mật khẩu WiFi.' : 'Mạng mở, không cần mật khẩu.';
    $('ssidField').hidden = !hidden;
    $('passField').hidden = !hidden && !network.secure;
    $('formError').hidden = true;
    $('passInput').value = '';
    $('ssidInput').value = '';
    show('joinView');
    var focus = hidden ? $('ssidInput') : network.secure ? $('passInput') : null;
    if (focus) setTimeout(function () { focus.focus(); }, 50);
  }

  function formError(message) {
    $('formError').textContent = message;
    $('formError').hidden = false;
  }

  function submit(event) {
    event.preventDefault();
    var ssid = selected ? selected.ssid : $('ssidInput').value.trim();
    var password = $('passInput').value;
    if (!ssid) { formError('Nhập tên mạng.'); return; }
    if (password && password.length < 8) { formError('Mật khẩu WiFi tối thiểu 8 ký tự.'); return; }
    $('joinBtn').disabled = true;
    $('waitLead').textContent = 'Máy đang vào mạng “' + ssid + '”. Mất khoảng 5–20 giây.';
    lastReplyAt = Date.now();
    show('waitView');
    var body = new URLSearchParams({ ssid: ssid, password: password });
    fetch('/connect', { method: 'POST', body: body }).catch(function () {});
    clearInterval(pollTimer);
    pollTimer = setInterval(poll, 1000);
  }

  function poll() {
    if (Date.now() - lastReplyAt > 10000) {
      $('waitLead').textContent = 'Điện thoại vừa bị ngắt khỏi ' + (apName || 'điểm phát của máy') +
        ' vì máy đang chuyển kênh WiFi. Hãy kết nối lại vào ' + (apName || 'điểm phát đó') +
        ' trong 20 giây để xem kết quả, hoặc xem trên màn hình LCD.';
    }
    fetch('/status').then(function (r) { return r.json(); }).then(function (d) {
      lastReplyAt = Date.now();
      if (d.deviceId) { $('deviceChip').textContent = d.deviceId; $('deviceChip').hidden = false; }
      if (d.state === 'connecting') return;
      clearInterval(pollTimer);
      $('joinBtn').disabled = false;
      if (d.state === 'connected') {
        $('doneId').textContent = d.deviceId;
        $('doneSsid').textContent = d.ssid;
        $('doneIp').textContent = d.ip;
        show('doneView');
      } else {
        show('joinView');
        formError(ERRORS[d.error] || 'Không kết nối được. Thử lại nhé.');
      }
    }).catch(function () {});
  }

  function loadDevice() {
    fetch('/status').then(function (r) { return r.json(); }).then(function (d) {
      if (d.deviceId) { $('deviceChip').textContent = d.deviceId; $('deviceChip').hidden = false; }
      if (d.ap) apName = d.ap;
    }).catch(function () {});
  }

  $('rescan').onclick = function () { scan(true); };
  $('otherBtn').onclick = function () { openJoin(null); };
  $('backBtn').onclick = function () { show('pickView'); };
  $('joinForm').onsubmit = submit;
  $('eyeBtn').onclick = function () {
    var input = $('passInput');
    var visible = input.type === 'password';
    input.type = visible ? 'text' : 'password';
    $('eyeBtn').classList.toggle('on', visible);
    $('eyeBtn').setAttribute('aria-label', visible ? 'Ẩn mật khẩu' : 'Hiện mật khẩu');
  };

  loadDevice();
  scan(true);
})();
