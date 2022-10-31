import pathlib

import pytest

from stoneadm.template import TemplateMgr, UndefinedError, TemplateNotFoundError


def test_render(stoneadm_module, fs):
    template_base = (pathlib.Path(__file__).parent / '../templates').resolve()
    fake_template = template_base / 'foo/bar'
    fs.create_file(fake_template, contents='{{ stoneadm_managed }}{{ var }}')

    template_mgr = TemplateMgr(stoneadm_module)
    value = 'test'

    # with base context
    expected_text = '{}{}'.format(template_mgr.base_context['stoneadm_managed'], value)
    assert template_mgr.render('foo/bar', {'var': value}) == expected_text

    # without base context
    with pytest.raises(UndefinedError):
        template_mgr.render('foo/bar', {'var': value}, managed_context=False)

    # override the base context
    context = {
        'stoneadm_managed': 'abc',
        'var': value
    }
    assert template_mgr.render('foo/bar', context) == 'abc{}'.format(value)

    # template not found
    with pytest.raises(TemplateNotFoundError):
        template_mgr.render('foo/bar/2', {})
